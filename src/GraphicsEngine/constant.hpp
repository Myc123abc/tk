#pragma once

#include <vulkan/vulkan.h>

#include <vector>

namespace tk { namespace graphics_engine {

inline constexpr auto Vulkan_Version = VK_API_VERSION_1_4;

inline const std::vector<const char*> Device_Extensions = 
{
  VK_KHR_SWAPCHAIN_EXTENSION_NAME,
};

inline constexpr uint32_t Max_Frame_Number = 2;

inline constexpr VkFormat Depth_Format = VK_FORMAT_D32_SFLOAT;

}}
