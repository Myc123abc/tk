#pragma once

#include <vulkan/vulkan.h>

#include "DescriptorLayout.hpp"
#include "Shader.hpp"
#include "PipelineLayout.hpp"

#include <vector>
#include <string>

namespace tk { namespace graphics_engine {

  struct ShaderCreateInfo
  {
    Shader&                            shader;
    VkShaderStageFlagBits              stage{};
    std::string                        shader_name;
    std::vector<VkDescriptorSetLayout> descriptor_layouts;
    std::vector<VkPushConstantRange>   push_constants;
  };

  class Device
  {
  public:
    operator VkDevice() const noexcept { return _device; }

    auto init(VkPhysicalDevice device, VkDeviceCreateInfo const& info) -> Device&;
    void destroy() noexcept;

    auto get() const noexcept { return _device; }

    void create_shaders(std::vector<ShaderCreateInfo> const& infos, bool link = false);

    auto create_descriptor_layout(std::vector<DescriptorInfo> const& infos) -> DescriptorLayout;

    auto create_pipeline_layout(
      std::vector<VkDescriptorSetLayout> const& descritptor_layouts = {},
      std::vector<VkPushConstantRange>   const& push_constants      = {}) -> PipelineLayout
    {
      return { _device, descritptor_layouts, push_constants };
    }

    auto& get_descriptor_buffer_info() const noexcept { return _descriptor_buffer_info; }

  private:
    VkDevice     _device = VK_NULL_HANDLE;
    VkPhysicalDeviceDescriptorBufferPropertiesEXT _descriptor_buffer_info{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_BUFFER_PROPERTIES_EXT };
  };

}}