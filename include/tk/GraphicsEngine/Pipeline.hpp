#pragma once

#include <vulkan/vulkan.h>

#include <vector>
#include <string_view>

namespace tk { namespace graphics_engine {

class Pipeline
{
public:
  Pipeline()  = default;
  ~Pipeline() = default;

  void destroy() noexcept;

  operator VkPipeline() const noexcept { return _pipeline; }

private:
  friend class Device;
  // TODO: add compute shader process
  Pipeline(Device& device, VkPipelineLayout layout, std::string_view vert, std::string_view frag, VkFormat format);

  auto create_shader_module(std::string_view shader) -> VkShaderModule;

private:
  Device*    _device;
  VkPipeline _pipeline{};
  VkFormat   _format{};
};

}}