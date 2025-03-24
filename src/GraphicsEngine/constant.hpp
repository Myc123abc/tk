#pragma once

#include <vulkan/vulkan.h>

#include <vector>

namespace tk
{

inline constexpr auto Vulkan_Version = VK_API_VERSION_1_4;

inline const std::vector<const char*> Device_Extensions = 
{
  VK_KHR_SWAPCHAIN_EXTENSION_NAME,
  // swapchain maintenance extension can auto recreate swapchain
  VK_EXT_SWAPCHAIN_MAINTENANCE_1_EXTENSION_NAME,
};

}
