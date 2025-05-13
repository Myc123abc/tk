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

    auto get() const noexcept { return _device; }

    auto create_descriptor_layout(std::vector<DescriptorInfo> const& infos) -> DescriptorLayout;

    auto create_pipeline(
      type::pipeline                            type,
      std::vector<std::string_view>      const& shaders,
      std::vector<VkDescriptorSetLayout> const& descritptor_layouts, 
      std::vector<VkPushConstantRange>   const& push_constants) -> Pipeline;
  
    auto& get_descriptor_buffer_info() const noexcept { return _descriptor_buffer_info; }

  private:
    VkDevice     _device = VK_NULL_HANDLE;
    VkPhysicalDeviceDescriptorBufferPropertiesEXT _descriptor_buffer_info{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_BUFFER_PROPERTIES_EXT };
  };

}}