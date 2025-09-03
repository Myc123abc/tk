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

  //uint32_t const buffer_size{ 1024 * 1024 };
  uint32_t const buffer_size{ 12788 };
  float    const buffer_expand_ratio{ 1.5 };
  uint32_t const max_descriptor_array_num{ 1 }; // TODO: change to dynamic allocate, this should just like x2 increase capacity in vector
                                                 // and this value only be like vector::reserve, not in config, should in creatation in renderpipeline

  bool use_descriptor_buffer{ false };
  bool use_shader_object{ true };
};

inline auto config() noexcept
{
  static Config cfg;
  return &cfg;
}

}}