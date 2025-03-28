#pragma once

#include "Log.hpp"
#include "ErrorHandling.hpp"
#include "Window.hpp"

#include <fmt/color.h>

#include <fstream>
#include <map>

namespace tk { namespace graphics_engine_init_util {


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

inline VkResult vkCreateDebugUtilsMessengerEXT(
  VkInstance                                  instance,
  VkDebugUtilsMessengerCreateInfoEXT const*   pCreateInfo,
  VkAllocationCallbacks const*                pAllocator,
  VkDebugUtilsMessengerEXT*                   pMessenger)
{
  auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance,"vkCreateDebugUtilsMessengerEXT");
  if (func != nullptr) 
    return func(instance, pCreateInfo, pAllocator, pMessenger);
  return VK_ERROR_EXTENSION_NOT_PRESENT;
}

inline void vkDestroyDebugUtilsMessengerEXT(
  VkInstance                                  instance,
  VkDebugUtilsMessengerEXT                    messenger,
  VkAllocationCallbacks const*                pAllocator)
{
  auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
  if (func != nullptr)
    func(instance, messenger, pAllocator);
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
  fmt::print(fg(fmt::color::green), "available instance layers:\n");
  for (const auto& layer : layers)
    fmt::print(fg(fmt::color::green), "  {}\n", layer.layerName);
  fmt::println("");
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
    throw_if(it == supported_layers.end(), fmt::format("unsupported layer: {}", layer));
  }
}


////////////////////////////////////////////////////////////////////////////////
//                              Extensions 
////////////////////////////////////////////////////////////////////////////////

inline auto get_instance_extensions()
{
  std::vector<const char*> extensions =
  {
    // VK_EXT_swapchain_maintenance_1 extension need these
    VK_KHR_GET_SURFACE_CAPABILITIES_2_EXTENSION_NAME,
    VK_EXT_SURFACE_MAINTENANCE_1_EXTENSION_NAME,
  };

  // glfw extensions
  uint32_t count;
  auto glfw_extensions = glfwGetRequiredInstanceExtensions(&count);
  extensions.insert(extensions.end(), glfw_extensions, glfw_extensions + count);

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
  fmt::print(fg(fmt::color::green), "available extensions:\n");
  for (const auto& extension : extensions)
    fmt::print(fg(fmt::color::green), "  {}\n", extension.extensionName);
  fmt::println("");
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
    throw_if(it == supported_extensions.end(), fmt::format("unsupported extension: {}", extension));
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
  fmt::print(fg(fmt::color::green),
             "available physical devices:\n"
             "  name\t\t\t\t\tscore\n");
  VkPhysicalDeviceProperties property;
  for (const auto& [score, device] : devices_score)
  {
    vkGetPhysicalDeviceProperties(device, &property);
    fmt::print(fg(fmt::color::green),
               "  {}\t{}\n", property.deviceName, score);
  }
  fmt::println("");
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
  fmt::print(fg(fmt::color::green), "available device extensions:\n");
  for (const auto& extension : get_supported_device_extensions(device))
    fmt::print(fg(fmt::color::green), "  {}\n", extension.extensionName);
  fmt::println("");
}

inline auto check_device_extensions_support(VkPhysicalDevice device, std::vector<const char*> const& extensions)
{
  auto supported_extensions = get_supported_device_extensions(device);
  for (auto extension : extensions) 
  {
      auto it = std::find_if(supported_extensions.begin(), supported_extensions.end(),
                             [extension] (const auto& supported_extension)
                             {
                               return strcmp(extension, supported_extension.extensionName) == 0;
                             });
      if (it == supported_extensions.end())
        return false;
  }
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
                             return format.format == VK_FORMAT_B8G8R8A8_SRGB &&
                                    format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
                           });
    if (it != formats.end())
      return *it;
    return formats[0];
  }
  
  auto get_present_mode()
  {
    auto it = std::find_if(present_modes.begin(), present_modes.end(),
                           [](const auto& mode)
                           {
                             return mode == VK_PRESENT_MODE_MAILBOX_KHR;
                           });
    if (it != present_modes.end())
      return *it;
    return VK_PRESENT_MODE_FIFO_KHR;
  }
  
  auto get_swap_extent(Window const& window)
  {
    if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
      return capabilities.currentExtent;
  
    int width, height;
    window.get_framebuffer_size(width, height);
  
    VkExtent2D actual_extent
    {
      (uint32_t)width,
      (uint32_t)height,
    };
  
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
  fmt::print(fg(fmt::color::green), "present mode:\t\t");
  if (present_mode == VK_PRESENT_MODE_FIFO_KHR)
    fmt::print(fg(fmt::color::green), "V-Sync");
  else if (present_mode == VK_PRESENT_MODE_MAILBOX_KHR)
    fmt::print(fg(fmt::color::green), "Mailbox");
  fmt::println("");
}


////////////////////////////////////////////////////////////////////////////////
//                               Image 
////////////////////////////////////////////////////////////////////////////////

inline VkImageView create_image_view(VkDevice device, VkImage image, VkFormat format)
{
  VkImageViewCreateInfo view_info
  {
    .sType            = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
    .image            = image,
    .viewType         = VK_IMAGE_VIEW_TYPE_2D,
    .format           = format,
    .subresourceRange =
    {
      .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
      .levelCount = 1,
      .layerCount = 1,
    },
  };
  VkImageView view;
  throw_if(vkCreateImageView(device, &view_info, nullptr, &view) != VK_SUCCESS,
          "failed to create image view");
  return view;
}


////////////////////////////////////////////////////////////////////////////////
//                               Shader 
////////////////////////////////////////////////////////////////////////////////

inline auto get_file_data(std::string_view filename)
{
  std::ifstream file(filename.data(), std::ios::ate | std::ios::binary);
  throw_if(!file.is_open(), fmt::format("failed to open {}", filename));

  size_t file_size = (size_t)file.tellg();
  std::vector<char> buffer(file_size);
  
  file.seekg(0);
  file.read(buffer.data(), file_size);

  file.close();
  return buffer;
}

struct Shader
{
  VkShaderModule shader;

  Shader(VkDevice device, std::string_view filename)
    : _device(device)
  {
    auto data = get_file_data(filename);
    VkShaderModuleCreateInfo info
    {
      .sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
      .codeSize = data.size(),
      .pCode    = reinterpret_cast<const uint32_t*>(data.data()),
    };
    throw_if(vkCreateShaderModule(device, &info, nullptr, &shader) != VK_SUCCESS,
             fmt::format("failed to create shader from {}", filename));
  }

  ~Shader()
  {
    vkDestroyShaderModule(_device, shader, nullptr);
  }

private:
  VkDevice _device;
};

} }
