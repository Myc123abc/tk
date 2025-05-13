#include "tk/GraphicsEngine/Descriptor.hpp"
#include "tk/ErrorHandling.hpp"
#include "tk/GraphicsEngine/Device.hpp"
#include "tk/util.hpp"
#include "tk/GraphicsEngine/vk_extension.hpp"

#include <cassert>

using namespace tk::util;

namespace tk { namespace graphics_engine {

void DescriptorLayout::destroy() noexcept
{
  assert(_device && _layout);
  vkDestroyDescriptorSetLayout(_device->get(), _layout, nullptr);
  _device = nullptr;
  _layout = VK_NULL_HANDLE;
  _size   = 0;
  _descriptors.clear();
}

DescriptorLayout::DescriptorLayout(Device* device, std::vector<DescriptorInfo> const& layouts)
{
  assert(device && !layouts.empty());

  _device      = device;
  _descriptors = layouts;

  // convert to VkDescriptorSetLayoutBinding
  std::vector<VkDescriptorSetLayoutBinding> bindings;
  bindings.reserve(layouts.size());
  for (auto const& layout : layouts)
  {
    // TODO: expand type check
    assert(layout.type == VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER ||
           layout.type == VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);

    assert(layout.binding >= 0                                  &&
           layout.type    != VK_DESCRIPTOR_TYPE_MAX_ENUM        &&    
           layout.count   >  0                                  && 
           layout.stages  != VK_SHADER_STAGE_FLAG_BITS_MAX_ENUM &&
           layout.type == VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER ?
             layout.sampler    != VK_NULL_HANDLE &&
             layout.image_view != VK_NULL_HANDLE : true         ||
           layout.type == VK_DESCRIPTOR_TYPE_STORAGE_IMAGE ?
             layout.image_view != VK_NULL_HANDLE : true);

    bindings.emplace_back(VkDescriptorSetLayoutBinding
    {
      .binding            = (uint32_t)layout.binding,
      .descriptorType     = layout.type,
      .descriptorCount    = layout.count,
      .stageFlags         = layout.stages,
    });
  }

  // create descriptor set layout
  VkDescriptorSetLayoutCreateInfo info
  {
    .sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
    .flags        = VK_DESCRIPTOR_SET_LAYOUT_CREATE_DESCRIPTOR_BUFFER_BIT_EXT,
    .bindingCount = (uint32_t)layouts.size(),
    .pBindings    = bindings.data(),
  };
  throw_if(vkCreateDescriptorSetLayout(device->get(), &info, nullptr, &_layout) != VK_SUCCESS,
           "failed to create descriptor set layout");

  // get descriptor layout size
  vkGetDescriptorSetLayoutSizeEXT(device->get(), _layout, &_size);
  _size = align_size(_size, _device->get_descriptor_buffer_info().descriptorBufferOffsetAlignment);
}

auto get_image_sampler_info(DescriptorInfo const& desc)
{
  return VkDescriptorImageInfo
  {
    .sampler     = desc.sampler,
    .imageView   = desc.image_view,
    .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
  };
}

auto get_storage_info(DescriptorInfo const& desc)
{
  return VkDescriptorImageInfo
  {
    .imageView   = desc.image_view,
    .imageLayout = VK_IMAGE_LAYOUT_GENERAL,
  };
}

void DescriptorLayout::put_descriptors(Buffer const& buffer)
{
  // FIXME: no offset
  auto data = (char*)buffer.data();

  // put descriptors
  size_t       data_size;
  VkDeviceSize offset;
  for (auto i = 0; i < _descriptors.size(); ++i)
  {
    auto& desc = _descriptors[i];

    // get descriptor offset in layout
    vkGetDescriptorSetLayoutBindingOffsetEXT(_device->get(), _layout, i, &offset);
    data += offset;

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

    vkGetDescriptorEXT(_device->get(), &get_info, data_size, data);
  }
}

}}