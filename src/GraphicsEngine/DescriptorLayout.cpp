#include "tk/GraphicsEngine/DescriptorLayout.hpp"
#include "tk/ErrorHandling.hpp"
#include "tk/GraphicsEngine/Device.hpp"
#include "tk/util.hpp"
#include "tk/GraphicsEngine/vk_extension.hpp"
#include "tk/GraphicsEngine/config.hpp"

#include <cassert>
#include <unordered_map>

using namespace tk::util;

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

DescriptorLayout::DescriptorLayout(Device* device, std::vector<DescriptorInfo> const& layouts)
{
  assert(device && !layouts.empty());

  _device      = device;
  _descriptors = layouts;

  std::unordered_map<int, uint32_t> types;

  // convert to VkDescriptorSetLayoutBinding
  std::vector<VkDescriptorSetLayoutBinding> bindings;
  bindings.reserve(layouts.size());
  for (auto const& layout : layouts)
  {
    // TODO: expand type check
    assert(layout.type == VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER ||
           layout.type == VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);

    ++types[layout.type];

    bindings.emplace_back(VkDescriptorSetLayoutBinding
    {
      .binding         = (uint32_t)layout.binding,
      .descriptorType  = layout.type,
      .descriptorCount = layout.count,
      .stageFlags      = layout.stages,
    });
  }

  // create descriptor set layout
  VkDescriptorSetLayoutCreateInfo info
  {
    .sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
    .bindingCount = static_cast<uint32_t>(layouts.size()),
    .pBindings    = bindings.data(),
  };
  if (config()->use_descriptor_buffer)
    info.flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_DESCRIPTOR_BUFFER_BIT_EXT;
  throw_if(vkCreateDescriptorSetLayout(device->get(), &info, nullptr, &_layout) != VK_SUCCESS,
           "failed to create descriptor set layout");

  // if cannot use descriptor buffer, use lengcy descriptor pool and set
  if (config()->use_descriptor_buffer)
  {
    // get descriptor layout size
    vkGetDescriptorSetLayoutSizeEXT(device->get(), _layout, &_size);
    _size = align_size(_size, _device->get_descriptor_buffer_info().descriptorBufferOffsetAlignment);
  }
  else
  {
    // create descriptor pool
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
      .maxSets       = 1,
      .poolSizeCount = static_cast<uint32_t>(pool_sizes.size()),
      .pPoolSizes    = pool_sizes.data(),
    };
    throw_if(vkCreateDescriptorPool(device->get(), &pool_info, nullptr, &_pool) != VK_SUCCESS,
             "failed to create descriptor pool");

    // allocate descriptor set
    VkDescriptorSetAllocateInfo alloc_info
    {
      .sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
      .descriptorPool     = _pool,
      .descriptorSetCount = 1,
      .pSetLayouts        = &_layout,
    };
    throw_if(vkAllocateDescriptorSets(device->get(), &alloc_info, &_set) != VK_SUCCESS,
             "failed to allocate descriptor set");

    update();
  }
}

auto get_image_sampler_info(DescriptorInfo const& desc)
{
  return VkDescriptorImageInfo
  {
    .sampler     = desc.sampler,
    .imageView   = desc.image->view(),
    .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
  };
}

auto get_storage_info(DescriptorInfo const& desc)
{
  return VkDescriptorImageInfo
  {
    .imageView   = desc.image->view(),
    .imageLayout = VK_IMAGE_LAYOUT_GENERAL,
  };
}

void DescriptorLayout::upload(Buffer& buffer, std::string_view tag)
{
  _buffer = &buffer;
  _ptr = reinterpret_cast<char*>(buffer.data()) + buffer.size();
  buffer.add_tag(tag);
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
  if (config()->use_descriptor_buffer == false)
  {
    std::vector<VkDescriptorImageInfo> image_infos;
    image_infos.reserve(_descriptors.size());
    for (auto const& desc : _descriptors)
      image_infos.emplace_back(VkDescriptorImageInfo
      {
        .sampler     = desc.sampler,
        .imageView   = desc.image->view(),
        .imageLayout = desc.type == VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER ? VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
                                                                              : VK_IMAGE_LAYOUT_GENERAL,
      });
    
    std::vector<VkWriteDescriptorSet> writes;
    writes.reserve(_descriptors.size());
    for (auto i{ 0 }; i < _descriptors.size(); ++i)
    {
      auto const& desc = _descriptors[i];
      writes.emplace_back(VkWriteDescriptorSet
      {
        .sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .dstSet          = _set,
        .dstBinding      = static_cast<uint32_t>(desc.binding),
        .descriptorCount = desc.count,
        .descriptorType  = desc.type,
        .pImageInfo      = &image_infos[i],
      });
    }
    // TODO: the best choose is only update changed image descriptors
    vkUpdateDescriptorSets(_device->get(), static_cast<uint32_t>(writes.size()), writes.data(), 0, nullptr);
    return {};
  }

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