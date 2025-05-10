#include "tk/GraphicsEngine/Device.hpp"
#include "tk/ErrorHandling.hpp"

#include <cassert>

namespace tk { namespace graphics_engine {

void Device::destroy() noexcept
{
  assert(_device);
  vkDestroyDevice(_device, nullptr);
  _device = VK_NULL_HANDLE;
}

auto Device::init(VkPhysicalDevice device, VkDeviceCreateInfo const& info) -> Device&
{
  assert(device);
  throw_if(vkCreateDevice(device, &info, nullptr, &_device) != VK_SUCCESS,
           "failed to create device");
  return *this;
}

auto Device::create_descriptor_layout(std::vector<VkDescriptorSetLayoutBinding> const& layouts) -> DescriptorLayout
{
  return DescriptorLayout(_device, layouts);
}

auto Device::create_pipeline(
  type::pipeline                            type,
  std::vector<std::string_view>      const& shaders,
  std::vector<VkDescriptorSetLayout> const& descritptor_layouts, 
  std::vector<VkPushConstantRange>   const& push_constants) -> Pipeline
{
  assert(type == type::pipeline::compute ? shaders.size() == 1 : shaders.size() == 2);
  if (type == type::pipeline::compute)
    return Pipeline(_device, shaders.front(), descritptor_layouts, push_constants);
  return Pipeline(_device, shaders[0], shaders[1], descritptor_layouts, push_constants);
}

}}