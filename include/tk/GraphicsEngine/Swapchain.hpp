//
// swapchain
//

#pragma once

#include "../Window.hpp"
#include "MemoryAllocator.hpp"

#include <vulkan/vulkan.h>

#include <vector>
#include <algorithm>


namespace tk { namespace graphics_engine {

struct SwapchainDetails
{
  VkSurfaceCapabilitiesKHR        capabilities{};
  std::vector<VkSurfaceFormatKHR> formats;
  std::vector<VkPresentModeKHR>   present_modes;

  SwapchainDetails(VkPhysicalDevice device, VkSurfaceKHR surface)
  {
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &capabilities);

    uint32_t count;
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &count, nullptr);
    formats.resize(count);
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &count, formats.data());

    vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &count, nullptr);
    present_modes.resize(count);
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &count, present_modes.data());
  }

  auto has_empty()
  {
    return formats.empty() || present_modes.empty();
  }

  auto get_surface_format()
  {
    auto it = std::find_if(formats.begin(), formats.end(),
                           [](const auto& format)
                           {
                             return format.format     == VK_FORMAT_B8G8R8A8_UNORM          &&
                                    format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
                           });
    if (it != formats.end())
      return *it;
    return formats[0];
  }
  
  auto get_present_mode()
  {
    //auto it = std::find_if(present_modes.begin(), present_modes.end(),
    //                       [](const auto& mode)
    //                       {
    //                         return mode == VK_PRESENT_MODE_MAILBOX_KHR;
    //                       });
    //if (it != present_modes.end())
    //  return *it;
    return VK_PRESENT_MODE_FIFO_KHR;
  }
  
  auto get_swap_extent(Window const* window)
  {
    if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
      return capabilities.currentExtent;
  
    auto size = window->get_framebuffer_size();
    VkExtent2D actual_extent{ static_cast<uint32_t>(size.x), static_cast<uint32_t>(size.y) };
  
    actual_extent.width = std::clamp(actual_extent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
    actual_extent.height = std::clamp(actual_extent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);
  
    return actual_extent;
  }

};

class Swapchain
{
public:
  Swapchain() = default;

  void init(VkPhysicalDevice physical_device, VkDevice device, VkSurfaceKHR surface, Window const* window);
  void destroy();

  void set_images();
  void resize();

  auto size()   const noexcept { return _images.size(); }
  auto format() const noexcept { return _swapchain_create_info.imageFormat; }

  auto& image(uint32_t index) noexcept { return _images[index]; }

  auto get() const noexcept { return _swapchain; }

private:
  VkPhysicalDevice         _physical_device;
  VkDevice                 _device;
  VkSurfaceKHR             _surface;
  VkSwapchainKHR           _swapchain;
  std::vector<Image>       _images;
  Window const*            _window{};
  VkSwapchainCreateInfoKHR _swapchain_create_info{};
};

}}