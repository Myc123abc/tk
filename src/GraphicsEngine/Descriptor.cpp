#include "tk/GraphicsEngine/Descriptor.hpp"
#include "tk/ErrorHandling.hpp"

#include <cassert>

namespace tk { namespace graphics_engine {

void DescriptorLayout::destroy() noexcept
{
  assert(_device && _layout);
  vkDestroyDescriptorSetLayout(_device, _layout, nullptr);
  _device = VK_NULL_HANDLE;
  _layout = VK_NULL_HANDLE;
}

DescriptorLayout::DescriptorLayout(VkDevice device, std::vector<VkDescriptorSetLayoutBinding> const& layouts)
{
  assert(device && !layouts.empty());

  _device = device;

  VkDescriptorSetLayoutCreateInfo info
  {
    .sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
    .bindingCount = (uint32_t)layouts.size(),
    .pBindings    = layouts.data(),
  };
  throw_if(vkCreateDescriptorSetLayout(_device, &info, nullptr, &_layout) != VK_SUCCESS,
           "failed to create descriptor set layout");
}

}}