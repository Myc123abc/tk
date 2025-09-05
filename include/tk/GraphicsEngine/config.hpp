#pragma once

#include <vulkan/vulkan.h>

#include <vector>

namespace tk { namespace graphics_engine {

struct Config
{
  uint32_t const           vulkan_version{ VK_API_VERSION_1_4 };
  std::vector<const char*> device_extensions
  {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME,
    VK_EXT_DESCRIPTOR_BUFFER_EXTENSION_NAME,
    VK_EXT_SHADER_OBJECT_EXTENSION_NAME,
  };

  uint32_t const buffer_size{ 1024 * 1024 };
  float    const buffer_expand_ratio{ 1.5 };
  
  bool use_descriptor_buffer{ false };
  bool use_shader_object{ false };
};

inline auto config() noexcept
{
  static Config cfg;
  return &cfg;
}

}}