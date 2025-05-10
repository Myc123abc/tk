#pragma once

#include <vulkan/vulkan.h>

#include "../type.hpp"
#include "Pipeline.hpp"
#include "Descriptor.hpp"

#include <vector>
#include <string_view>

namespace tk { namespace graphics_engine {

  class Device
  {
  public:
    operator VkDevice() const noexcept { return _device; }

    auto init(VkPhysicalDevice device, VkDeviceCreateInfo const& info) -> Device&;
    void destroy() noexcept;

    auto create_descriptor_layout(std::vector<VkDescriptorSetLayoutBinding> const& layouts) -> DescriptorLayout;

    auto create_pipeline(
      type::pipeline                            type,
      std::vector<std::string_view>      const& shaders,
      std::vector<VkDescriptorSetLayout> const& descritptor_layouts, 
      std::vector<VkPushConstantRange>   const& push_constants) -> Pipeline;
  
  private:
    VkDevice _device = VK_NULL_HANDLE;
  };

}}