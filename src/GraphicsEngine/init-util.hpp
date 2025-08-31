#pragma once

#include "tk/log.hpp"
#include "tk/ErrorHandling.hpp"
#include "tk/Window.hpp"
#include "tk/GraphicsEngine/config.hpp"

#include <map>
#include <print>
#include <algorithm>

namespace tk { namespace graphics_engine {

////////////////////////////////////////////////////////////////////////////////
//                              Debug Messenger 
////////////////////////////////////////////////////////////////////////////////

inline VKAPI_ATTR VkBool32 VKAPI_CALL debug_messenger_callback(
  VkDebugUtilsMessageSeverityFlagBitsEXT      message_severity,
  VkDebugUtilsMessageTypeFlagsEXT             message_type,
  VkDebugUtilsMessengerCallbackDataEXT const* callback_data,
  void*                                       user_data)
{
  log::info(callback_data->pMessage);
  return VK_FALSE;
}

inline auto get_debug_messenger_create_info()
{
  return VkDebugUtilsMessengerCreateInfoEXT
  {
    .sType           = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
    .messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
                       VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                       VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,
    .messageType     = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT     |
                       VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT  |
                       VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
    .pfnUserCallback = debug_messenger_callback,
  };
}

////////////////////////////////////////////////////////////////////////////////
//                              Layers 
////////////////////////////////////////////////////////////////////////////////

inline auto get_supported_instance_layers()
{
  uint32_t count;
  vkEnumerateInstanceLayerProperties(&count, nullptr);
  std::vector<VkLayerProperties> layers(count);
  vkEnumerateInstanceLayerProperties(&count, layers.data());
  return layers;
}

inline auto print_supported_instance_layers()
{
  auto layers = get_supported_instance_layers();
  std::println("available instance layers:");
  for (const auto& layer : layers)
    std::println("  {}", layer.layerName);
  std::println();
}

inline auto check_layers_support(std::vector<std::string_view> const& layers)
{
  auto supported_layers = get_supported_instance_layers();

  for (auto layer : layers)
  {
    auto it = std::find_if(supported_layers.begin(), supported_layers.end(),
                           [layer](const auto& supported_layer)
                           {
                             return supported_layer.layerName == layer;
                           });
    throw_if(it == supported_layers.end(), "unsupported layer: {}", layer);
  }
}


////////////////////////////////////////////////////////////////////////////////
//                              Extensions 
////////////////////////////////////////////////////////////////////////////////

inline auto get_instance_extensions()
{
  std::vector<char const*> extensions;

  // instance extensions
  extensions.append_range(Window::get_vulkan_instance_extensions());

  // debug messenger extension
#ifndef NDEBUG
  extensions.emplace_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
#endif

  return extensions;
}

inline auto get_supported_instance_extensions()
{
  uint32_t count = 0;
  vkEnumerateInstanceExtensionProperties(nullptr, &count, nullptr);
  std::vector<VkExtensionProperties> extensions(count);
  vkEnumerateInstanceExtensionProperties(nullptr, &count, extensions.data());
  return extensions;
}

inline auto print_supported_instance_extensions()
{
  auto extensions = get_supported_instance_extensions();
  std::println("available instance extensions:");
  for (const auto& extension : extensions)
    std::println("  {}", extension.extensionName);
  std::println();
}

inline auto print_enabled_extensions(std::string_view header_msg, std::vector<const char*> const& extensions)
{
  std::println("Enabled {} extensions:", header_msg);
  for (const auto& extension : extensions)
    std::println("  {}", extension);
  std::println();
}

inline auto check_instance_extensions_support(std::vector<const char*> extensions)
{
  auto supported_extensions = get_supported_instance_extensions();
  for (auto extension : extensions)
  {
    auto it = std::find_if(supported_extensions.begin(), supported_extensions.end(),
                           [extension] (const auto& supported_extension) {
                             return strcmp(supported_extension.extensionName, extension) == 0;
                           });
    throw_if(it == supported_extensions.end(), "unsupported extension: {}", extension);
  }
}


////////////////////////////////////////////////////////////////////////////////
//                              Physical Device 
////////////////////////////////////////////////////////////////////////////////

inline auto get_supported_physical_devices(VkInstance instance)
{
  uint32_t count;
  vkEnumeratePhysicalDevices(instance, &count, nullptr);
  throw_if(count == 0, "failed to find GPUs with vulkan support");
  std::vector<VkPhysicalDevice> devices(count);
  vkEnumeratePhysicalDevices(instance, &count, devices.data());
  return devices;
}

inline auto get_physical_devices_score(std::vector<VkPhysicalDevice> const& devices)
{
  std::multimap<int, VkPhysicalDevice> devices_score;

  VkPhysicalDeviceProperties properties;
  VkPhysicalDeviceVulkan13Features features13{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES };
  VkPhysicalDeviceFeatures2 features2
  {
    .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2,
    .pNext = &features13
  };
  features2.pNext = &features13;
  VkPhysicalDeviceDescriptorBufferFeaturesEXT descriptor_buffer_features
  {
    .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_BUFFER_FEATURES_EXT,
  };
  features13.pNext = &descriptor_buffer_features;

  for (const auto& device : devices)
  {
    int score = 0;
    vkGetPhysicalDeviceProperties(device, &properties);
    vkGetPhysicalDeviceFeatures2(device, &features2);
    if (properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
      score += 1000;
    score += properties.limits.maxImageDimension2D;
    if (!features2.features.geometryShader    &&
        !features2.features.samplerAnisotropy &&
        !features13.synchronization2)
      score = 0;
    devices_score.insert(std::make_pair(score, device));
  }

  return devices_score;
}

inline void print_supported_physical_devices(VkInstance instance)
{
  auto devices = get_supported_physical_devices(instance);
  auto devices_score = get_physical_devices_score(devices);
  std::println("available physical devices:\n"
               "  name\t\t\t\t\tscore");
  VkPhysicalDeviceProperties property;
  for (const auto& [score, device] : devices_score)
  {
    vkGetPhysicalDeviceProperties(device, &property);
    std::println("  {}\t{}", property.deviceName, score);
  }
  std::println();
}

inline auto get_supported_device_extensions(VkPhysicalDevice device)
{
  uint32_t count;
  vkEnumerateDeviceExtensionProperties(device, nullptr, &count, nullptr);
  std::vector<VkExtensionProperties> extensions(count);
  vkEnumerateDeviceExtensionProperties(device, nullptr, &count, extensions.data());
  return extensions;
}

inline auto print_supported_device_extensions(VkPhysicalDevice device)
{
  std::println("available device extensions:");
  for (const auto& extension : get_supported_device_extensions(device))
    std::println("  {}", extension.extensionName);
  std::println();
}

inline auto check_device_extensions_support(VkPhysicalDevice device, std::vector<char const*>& extensions)
{
  std::vector<char const*> supported_extensions;

  auto available_extensions = get_supported_device_extensions(device);
  for (auto extension : extensions) 
  {
      auto it = std::find_if(available_extensions.begin(), available_extensions.end(),
                             [extension] (const auto& supported_extension)
                             {
                               return strcmp(extension, supported_extension.extensionName) == 0;
                             });
      if (it == available_extensions.end())
      {
        if (strcmp(extension, VK_EXT_DESCRIPTOR_BUFFER_EXTENSION_NAME) == 0)
        {
          config()->use_descriptor_buffer = false;
#ifndef NDEBUG
          log::warn("descriptor buffer extension is not supported, fallback to descriptor pool and set");
#endif
          continue;
        }
        if (strcmp(extension, VK_EXT_SHADER_OBJECT_EXTENSION_NAME) == 0)
        {
          config()->use_shader_object = false;
#ifndef NDEBUG
          log::warn("shader object extension is not supported, fallback to pipeline");
#endif
          continue;
        }
        return false;
      }

      supported_extensions.push_back(extension);
  }
  extensions = std::move(supported_extensions);
  return true;
}

////////////////////////////////////////////////////////////////////////////////
//                                Queue Families 
////////////////////////////////////////////////////////////////////////////////

inline auto get_supported_queue_families(VkPhysicalDevice device)
{
  uint32_t count;
  vkGetPhysicalDeviceQueueFamilyProperties(device, &count, nullptr);
  std::vector<VkQueueFamilyProperties> families(count);
  vkGetPhysicalDeviceQueueFamilyProperties(device, &count, families.data());
  return families;
}

struct QueueFamilyIndices
{
  std::optional<uint32_t> graphics_family;
  std::optional<uint32_t> present_family;

  auto has_all_queue_families()
  {
    return graphics_family.has_value() &&
           present_family.has_value();
  }
};

inline auto get_queue_family_indices(VkPhysicalDevice device, VkSurfaceKHR surface)
{
  auto queue_families = get_supported_queue_families(device);
  std::vector<QueueFamilyIndices> all_indices;
  VkBool32 WSI_support;
  for (uint32_t i = 0; i < queue_families.size(); ++i)
  {
    QueueFamilyIndices indices;

    if (queue_families[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
      indices.graphics_family = i;
    
    vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &WSI_support);
    if (WSI_support)
      indices.present_family = i;

    if (indices.has_all_queue_families())
      all_indices.emplace_back(indices);
  }

  throw_if(all_indices.empty(), "failed to support necessary queue families");

  // some queue features may be in a same index,
  // so less queues best performance when queue is not much
  auto it = std::find_if(all_indices.begin(), all_indices.end(),
    [](const auto& indicies)
    {
      return indicies.graphics_family.value() == indicies.present_family.value();
    });
  if (it == all_indices.end())
    return all_indices[0];
  return *it;
}


////////////////////////////////////////////////////////////////////////////////
//                                Swapchain 
////////////////////////////////////////////////////////////////////////////////

struct SwapChainSupportDetails
{
  VkSurfaceCapabilitiesKHR        capabilities;
  std::vector<VkSurfaceFormatKHR> formats;
  std::vector<VkPresentModeKHR>   present_modes;

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
  
  auto get_swap_extent(Window const& window)
  {
    if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
      return capabilities.currentExtent;
  
    auto size = window.get_framebuffer_size();
    VkExtent2D actual_extent{ static_cast<uint32_t>(size.x), static_cast<uint32_t>(size.y) };
  
    actual_extent.width = std::clamp(actual_extent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
    actual_extent.height = std::clamp(actual_extent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);
  
    return actual_extent;
  }

};

inline auto get_swapchain_details(VkPhysicalDevice device, VkSurfaceKHR surface)
{
  SwapChainSupportDetails details;

  vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.capabilities);

  uint32_t count;
  vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &count, nullptr);
  details.formats.resize(count);
  vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &count, details.formats.data());

  vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &count, nullptr);
  details.present_modes.resize(count);
  vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &count, details.present_modes.data());

  return details;
}

inline void print_present_mode(VkPresentModeKHR present_mode)
{
  std::print("present mode:\t\t");
  if (present_mode == VK_PRESENT_MODE_FIFO_KHR)
    std::print("V-Sync");
  else if (present_mode == VK_PRESENT_MODE_MAILBOX_KHR)
    std::print("Mailbox");
  std::println();
}

} }
