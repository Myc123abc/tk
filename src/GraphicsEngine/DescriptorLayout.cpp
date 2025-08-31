#include "tk/GraphicsEngine/DescriptorLayout.hpp"
#include "tk/ErrorHandling.hpp"
#include "tk/GraphicsEngine/Device.hpp"
#include "tk/util.hpp"
#include "tk/GraphicsEngine/vk_extension.hpp"
#include "tk/GraphicsEngine/config.hpp"

#include <cassert>
#include <unordered_map>
#include <unordered_set>
#include <format>

using namespace tk::util;

using namespace tk;
using namespace tk::graphics_engine;

namespace
{

inline void throw_if(bool b, std::string_view msg)
{
  if (b) throw std::exception(std::format("[Descriptor] {}", msg).data());
}

void check_descriptor_infos_valid(std::vector<DescriptorInfo> const& layouts)
{
  auto bindings = std::unordered_set<uint32_t>(layouts.size());
  auto types    = std::unordered_set<VkDescriptorType>(layouts.size());

  // TODO: assume only single variable descriptors can be used now
  bool has_single_variable_descriptor{};

  for (auto const& layout : layouts)
  {
    throw_if(bindings.emplace(layout.binding).second == false,
             "descriptor bindings duplicate!");
    throw_if(types.emplace(layout.type).second == false,
             "descriptor types duplicate!");

    if (layout.images.size() > 1)
    {
      throw_if(has_single_variable_descriptor,
               "assume only single variable descriptors can be used now");
      has_single_variable_descriptor = true;
    }

    // TODO: assume all use combined image now
    throw_if(layout.type != VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
             "assume all use combined image now");
  }
}

}

namespace tk { namespace graphics_engine {

void DescriptorLayout::destroy() noexcept
{
  assert(_device && _layout);
  vkDestroyDescriptorSetLayout(_device->get(), _layout, nullptr);
  if (config()->use_descriptor_buffer == false)
    vkDestroyDescriptorPool(_device->get(), _pool, nullptr);
  _device          = {};
  _layout          = {};
  _size            = {};
  _pipeline_layout = {};
  _bind_point      = {};
  _buffer          = {};
  _tag             = {};
  _descriptors.clear();
  _pool            = {};
  _set             = {};
}

DescriptorLayout::DescriptorLayout(Device* device, std::vector<DescriptorInfo> const& descriptor_infos)
{
  assert(device && !descriptor_infos.empty());

  _device      = device;
  _descriptors = descriptor_infos; // TODO: maybe discard

#ifndef NDEBUG // for performance consider, only debug enable check
  check_descriptor_infos_valid(descriptor_infos);
#endif

  if (config()->use_descriptor_buffer)
  {

  }
  else
  {
    // TODO:
    //       2. dynamic pool
    create_descriptor_pool(descriptor_infos);
    create_descriptor_set_layout(descriptor_infos);    
    allocate_descriptor_sets(get_variable_descriptor_count(descriptor_infos));
    update_descriptor_sets(descriptor_infos);
    return; // TODO: discard after finish
  }

  // TODO: discard after finish
  {
  assert(true);
#if 0
#error disable descriptor buffer now
  std::unordered_map<int, uint32_t> types;

  // convert to VkDescriptorSetLayoutBinding
  std::vector<VkDescriptorSetLayoutBinding> bindings;
  bindings.reserve(layouts.size());
  for (auto const& layout : layouts)
  {
    // TODO: expand type check
    assert(layout.type == VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER ||
           layout.type == VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);

    auto count = layout.use_array ? config()->max_descriptor_array_num : 1;
    types[layout.type] += count;

    bindings.emplace_back(VkDescriptorSetLayoutBinding
    {
      .binding         = (uint32_t)layout.binding,
      .descriptorType  = layout.type,
      .descriptorCount = count,
      .stageFlags      = layout.stages,
    });
  }

  create_descrptor_set_layout(bindings);

  // if cannot use descriptor buffer, use lengcy descriptor pool and set
  if (config()->use_descriptor_buffer)
  {
    // get descriptor layout size
    vkGetDescriptorSetLayoutSizeEXT(device->get(), _layout, &_size);
    _size = align_size(_size, _device->get_descriptor_buffer_info().descriptorBufferOffsetAlignment);
  }
  else
    create_descriptor_pool(types);
#endif
  }
}

void DescriptorLayout::create_descriptor_pool(std::vector<DescriptorInfo> const& descriptor_infos)
{
  auto pool_sizes  = std::vector<VkDescriptorPoolSize>();
  auto create_info = VkDescriptorPoolCreateInfo{ VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO };

  // add pool size of every descriptor type
  for (auto const& descriptor_info : descriptor_infos)
  {
    pool_sizes.emplace_back(VkDescriptorPoolSize
    {
      .type            = descriptor_info.type,
	    .descriptorCount = static_cast<uint32_t>(descriptor_info.images.size()),
    });
  };

  // fill create info
  create_info.maxSets       = 1, // TODO: only single set now
  create_info.poolSizeCount = static_cast<uint32_t>(pool_sizes.size()),
  create_info.pPoolSizes    = pool_sizes.data(),

  // create descriptor pool
  throw_if(vkCreateDescriptorPool(_device->get(), &create_info, nullptr, &_pool) != VK_SUCCESS,
           "failed to create descriptor pool");
}

void DescriptorLayout::create_descriptor_set_layout(std::vector<DescriptorInfo> const& descriptor_infos)
{
  auto bindings                  = std::vector<VkDescriptorSetLayoutBinding>();
  auto binding_flags             = std::vector<VkDescriptorBindingFlags>();
  auto binding_flags_create_info = VkDescriptorSetLayoutBindingFlagsCreateInfo{ VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO };
  auto layout_create_info        = VkDescriptorSetLayoutCreateInfo{ VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO };
  
  // add descriptor binding info
  for (auto const& descriptor_info : descriptor_infos)
  {
    bindings.emplace_back(VkDescriptorSetLayoutBinding
    {
      .binding         = static_cast<uint32_t>(descriptor_info.binding),
      .descriptorType  = descriptor_info.type,
      .descriptorCount = static_cast<uint32_t>(descriptor_info.images.size()), // TODO: maybe use dynamic increasing like vector in 2x
      .stageFlags      = descriptor_info.stages,
    });
    // whether this binding is variable-sized descriptor
    binding_flags.emplace_back(descriptor_info.images.size() > 1 ? VK_DESCRIPTOR_BINDING_VARIABLE_DESCRIPTOR_COUNT_BIT : 0);
  };
  
  // fill binding flags create info
  binding_flags_create_info.bindingCount  = static_cast<uint32_t>(binding_flags.size()),
  binding_flags_create_info.pBindingFlags = binding_flags.data();

  // fill layout create info
  layout_create_info.pBindings    = bindings.data();
  layout_create_info.bindingCount = static_cast<uint32_t>(bindings.size());
  layout_create_info.pNext        = &binding_flags_create_info;
    
  // create descriptor set layout
  throw_if(vkCreateDescriptorSetLayout(_device->get(), &layout_create_info, nullptr, &_layout) != VK_SUCCESS,
           "failed to create descriptor set layout");
}

void DescriptorLayout::allocate_descriptor_sets(uint32_t variable_descriptor_count)
{
  auto variable_descriptor_counts              = std::vector<uint32_t>();
  auto variable_descriptor_count_allocate_info = VkDescriptorSetVariableDescriptorCountAllocateInfo{ VK_STRUCTURE_TYPE_DESCRIPTOR_SET_VARIABLE_DESCRIPTOR_COUNT_ALLOCATE_INFO };
  auto descriptor_set_allocate_info            = VkDescriptorSetAllocateInfo{ VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO };

  // add variable descriptor counts
  variable_descriptor_counts = { variable_descriptor_count };  // TODO: only single descriptor set's variable descriptor counts using now,
                                                               //       and I dont't know whether all variable-sized descriptors
                                                               //       sharing single variable descriptor counts per descriptor set

  // fill variable count allocate info
  variable_descriptor_count_allocate_info.descriptorSetCount = static_cast<uint32_t>(variable_descriptor_counts.size());
  variable_descriptor_count_allocate_info.pDescriptorCounts  = variable_descriptor_counts.data();

  // fill descriptor set allocate info
  descriptor_set_allocate_info.descriptorPool     = _pool;
  descriptor_set_allocate_info.pSetLayouts        = &_layout;
  descriptor_set_allocate_info.descriptorSetCount = 1; // TODO: single set now
  descriptor_set_allocate_info.pNext              = &variable_descriptor_count_allocate_info;

  // allocate descriptor sets
  throw_if(vkAllocateDescriptorSets(_device->get(), &descriptor_set_allocate_info, &_set) != VK_SUCCESS,
           "failed to allocate descriptor set");
}

void DescriptorLayout::update_descriptor_sets(std::vector<DescriptorInfo> const& descriptor_infos)
{
  auto image_infos = std::vector<VkDescriptorImageInfo>();
  auto writes      = std::vector<VkWriteDescriptorSet>(descriptor_infos.size());

  // resize image infos
  // TODO: assume only single descriptor and use combined image
  throw_if(descriptor_infos.size() != 1 || descriptor_infos[0].images.empty(),
           "assume only single descriptor and use combined image");
  image_infos.resize(descriptor_infos[0].images.size());

  // fill image descriptor infos
  for (auto i = 0; i < image_infos.size(); ++i)
  {
    // TODO: assume all images are combined_image
    throw_if(descriptor_infos[0].type != VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
             "assume all images are combined image");
    image_infos[i] =
    {
      .sampler     = descriptor_infos[0].sampler,
      .imageView   = descriptor_infos[0].images[i]->view(),
      .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, // TODO: assume all images are combined_image
    };
  }

  // fill descriptor set writes
  for (auto i = 0; i < descriptor_infos.size(); ++i)
  {
    auto& descriptor_info = descriptor_infos[i];
    writes[i] =
    {
      .sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
      .dstSet          = _set, // TODO: only single set now
      .dstBinding      = descriptor_info.binding,
      .descriptorCount = static_cast<uint32_t>(descriptor_info.images.size()),
      .descriptorType  = descriptor_info.type,
      .pImageInfo      = image_infos.data(), // TODO: assume only single descriptor set and only image
    };
  }

  // update descriptor sets
  vkUpdateDescriptorSets(_device->get(), static_cast<uint32_t>(writes.size()), writes.data(), 0, nullptr);
}

auto DescriptorLayout::get_variable_descriptor_count(std::vector<DescriptorInfo> const& descriptor_infos) -> uint32_t
{
  // TODO: assume only single variable descriptor now
  auto it = std::ranges::find_if(descriptor_infos, [](auto const& info) { return info.images.size() > 1; });
  return it != descriptor_infos.end() ? it->images.size() : 0;
}

void DescriptorLayout::create_descrptor_set_layout(std::vector<VkDescriptorSetLayoutBinding> const& bindings)
{
  std::vector<VkDescriptorBindingFlagsEXT> flags;

  VkDescriptorSetLayoutCreateInfo info
  {
    .sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
    .bindingCount = static_cast<uint32_t>(_descriptors.size()),
    .pBindings    = bindings.data(),
  };
  if (config()->use_descriptor_buffer)
    // use descriptor buffer
    info.flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_DESCRIPTOR_BUFFER_BIT_EXT;
  else
  {
    // descriptor sets need to enable dynamic descriptor array feature
    //info.flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT;
    for (auto const& binding : bindings)
      flags.emplace_back(binding.descriptorCount == 0 ? 0 : VK_DESCRIPTOR_BINDING_VARIABLE_DESCRIPTOR_COUNT_BIT);
      //VK_DESCRIPTOR_BINDING_VARIABLE_DESCRIPTOR_COUNT_BIT_EXT   |
      //VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT_EXT             |
      //VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT_EXT           |
      //VK_DESCRIPTOR_BINDING_UPDATE_UNUSED_WHILE_PENDING_BIT_EXT;
    VkDescriptorSetLayoutBindingFlagsCreateInfo binding_flags
    {
      .sType          = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO,
      .bindingCount   = static_cast<uint32_t>(flags.size()),
      .pBindingFlags  = flags.data(),
    };
    //info.pNext = &binding_flags;
  }
  throw_if(vkCreateDescriptorSetLayout(_device->get(), &info, nullptr, &_layout) != VK_SUCCESS,
           "failed to create descriptor set layout");
}

void DescriptorLayout::create_descriptor_pool(std::unordered_map<int, uint32_t> const& types)
{
  std::vector<VkDescriptorPoolSize> pool_sizes;
  pool_sizes.reserve(types.size());
  for (auto const& [type, count] : types)
    pool_sizes.emplace_back(VkDescriptorPoolSize
    {
      .type            = static_cast<VkDescriptorType>(type),
      .descriptorCount = count,
    });
  VkDescriptorPoolCreateInfo pool_info
  {
    .sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
    //.flags         = VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT,
    .maxSets       = 1,
    .poolSizeCount = static_cast<uint32_t>(pool_sizes.size()),
    .pPoolSizes    = pool_sizes.data(),
  };
  throw_if(vkCreateDescriptorPool(_device->get(), &pool_info, nullptr, &_pool) != VK_SUCCESS,
           "failed to create descriptor pool");

  // calculate variable descriptor count
  // TODO: is variable-sized descriptor count is shared in set layout for all variable descriptors?
  uint32_t variable_count{};
  for (auto const& [_, count] : types)
    if (count > 1) variable_count += count;
  VkDescriptorSetVariableDescriptorCountAllocateInfo variable_descriptor_count_alloc_info
  {
    .sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_VARIABLE_DESCRIPTOR_COUNT_ALLOCATE_INFO,
    .descriptorSetCount = 1,
    .pDescriptorCounts  = &variable_count,
  };

  // allocate descriptor set
  VkDescriptorSetAllocateInfo alloc_info
  {
    .sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
    //.pNext              = variable_count ? &variable_descriptor_count_alloc_info : nullptr,
    .descriptorPool     = _pool,
    .descriptorSetCount = 1, // TODO: I don't know how to use multiple sets
    .pSetLayouts        = &_layout,
  };
  throw_if(vkAllocateDescriptorSets(_device->get(), &alloc_info, &_set) != VK_SUCCESS,
           "failed to allocate descriptor set");
  update();
}

auto get_image_sampler_info(DescriptorInfo const& desc)
{
  assert(true);
  return VkDescriptorImageInfo
  {
    .sampler     = desc.sampler,
    //.imageView   = desc.image->view(),
    .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
  };
}

auto get_storage_info(DescriptorInfo const& desc)
{
  assert(true);
  return VkDescriptorImageInfo
  {
    //.imageView   = desc.image->view(),
    .imageLayout = VK_IMAGE_LAYOUT_GENERAL,
  };
}

void DescriptorLayout::upload(Buffer& buffer, std::string_view tag)
{
  _buffer = &buffer;
  _ptr = reinterpret_cast<char*>(buffer.data()) + buffer.size();
  buffer.add_tag(tag.data());
  _tag = tag;
  buffer.add_size(align_size(update(), _device->get_descriptor_buffer_info().descriptorBufferOffsetAlignment));
}

void bind_descriptor_buffer(Command& cmd, Buffer const& buffer)
{
  // bind descriptor buffer
  VkDescriptorBufferBindingInfoEXT info
  {
    .sType   = VK_STRUCTURE_TYPE_DESCRIPTOR_BUFFER_BINDING_INFO_EXT,
    .address = buffer.address(),
    .usage   = buffer.descriptor_buffer_usages(),
  };
  vkCmdBindDescriptorBuffersEXT(cmd, 1, &info);
}

void DescriptorLayout::bind(Command& cmd)
{
  if (config()->use_descriptor_buffer == false)
  {
    vkCmdBindDescriptorSets(cmd, _bind_point, _pipeline_layout, 0, 1, &_set, 0, nullptr);
    return;
  }

  // set offset in buffer
  VkDeviceSize offset{ _buffer->offset(_tag) };
  uint32_t buffer_index{};
  vkCmdSetDescriptorBufferOffsetsEXT(cmd, _bind_point, _pipeline_layout, 0, 1, &buffer_index, &offset);
}

auto DescriptorLayout::update() -> uint32_t
{
  if (config()->use_descriptor_buffer)
  {
    return update_descriptor_buffer();
  }
  else
  {
    update_descriptor_sets();
    return {};
  }
}

void DescriptorLayout::update_descriptor_sets()
{
  std::vector<VkDescriptorImageInfo> image_infos;
  image_infos.reserve(_descriptors.size());
  assert(true);
  for (auto const& desc : _descriptors)
    image_infos.emplace_back(VkDescriptorImageInfo
    {
      .sampler     = desc.sampler,
      //.imageView   = desc.image->view(),
      .imageLayout = desc.type == VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER ? VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
                                                                            : VK_IMAGE_LAYOUT_GENERAL,
    });
    
  std::vector<VkWriteDescriptorSet> writes;
  writes.reserve(_descriptors.size());
  for (auto i{ 0 }; i < _descriptors.size(); ++i)
  {
    auto const& desc = _descriptors[i];
    assert(true);
    writes.emplace_back(VkWriteDescriptorSet
    {
      .sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
      .dstSet          = _set,
      .dstBinding      = static_cast<uint32_t>(desc.binding),
      //.descriptorCount = desc.use_array ? config()->max_descriptor_array_num : 1,
      .descriptorType  = desc.type,
      .pImageInfo      = &image_infos[i],
    });
  }
  // TODO: the best choose is only update changed image descriptors
  vkUpdateDescriptorSets(_device->get(), static_cast<uint32_t>(writes.size()), writes.data(), 0, nullptr);
}

auto DescriptorLayout::update_descriptor_buffer() -> uint32_t
{
  // put descriptors
  size_t       data_size{};
  VkDeviceSize offset{};
  for (auto const& desc : _descriptors)
  {
    // get descriptor offset in layout
    vkGetDescriptorSetLayoutBindingOffsetEXT(_device->get(), _layout, desc.binding, &offset);

    VkDescriptorGetInfoEXT get_info
    {
      .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_GET_INFO_EXT,
      .type  = desc.type,
    };
    
    VkDescriptorImageInfo img_info;

    switch (desc.type)
    {
      default:
        // TODO: wait for expand
        assert(false);

      case VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:
        img_info = get_image_sampler_info(desc);
        get_info.data.pCombinedImageSampler = &img_info;
        data_size = _device->get_descriptor_buffer_info().combinedImageSamplerDescriptorSize;
        break;

      case VK_DESCRIPTOR_TYPE_STORAGE_IMAGE:
        img_info = get_storage_info(desc);
        get_info.data.pStorageImage = &img_info;
        data_size = _device->get_descriptor_buffer_info().storageImageDescriptorSize;
        break;
    }

    // FIXME: multiple descriptor bindings maybe error, check vkGetDescriptorSetLayoutBindingOffsetEXT and vkGetDescriptorEXT
    //        and maybe offset also need change
    vkGetDescriptorEXT(_device->get(), &get_info, data_size, _ptr + offset);
    offset += data_size;
  }
  return offset;
}

}}