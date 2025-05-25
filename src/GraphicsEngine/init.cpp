#include "tk/GraphicsEngine/GraphicsEngine.hpp"
#include "init-util.hpp"
#include "tk/GraphicsEngine/vk_extension.hpp"

#include <ranges>
#include <set>
#include <print>

namespace tk { namespace graphics_engine { 

static constexpr auto          Vulkan_Version    = VK_API_VERSION_1_4;
std::vector<const char*> const Device_Extensions = 
{
  VK_KHR_SWAPCHAIN_EXTENSION_NAME,
  VK_EXT_DESCRIPTOR_BUFFER_EXTENSION_NAME,
  VK_EXT_SHADER_OBJECT_EXTENSION_NAME,
};

void GraphicsEngine::init(Window& window)
{
  // only have single graphics engine
  static bool first = true;
  assert(first);
  if (first)  first = false;

  _window = &window;

  create_instance();
  load_instance_extension_funcs();
#ifndef NDEBUG
  create_debug_messenger();
#endif
  create_surface();
  select_physical_device();
  create_device_and_get_queues();
  load_device_extension_funcs();
  create_swapchain();
  init_command_pool();
  init_memory_allocator();
  create_frame_resources();

  create_sdf_rendering_resource();

  _window->show();
}

void GraphicsEngine::destroy()
{
  vkDeviceWaitIdle(_device);
  _destructors.clear();
}

void GraphicsEngine::create_instance()
{
  // debug messenger
#ifndef NDEBUG
  auto debug_messenger_info = get_debug_messenger_create_info();
#endif

  // app info
  VkApplicationInfo app_info
  {
    .pApplicationName = "tk",
    .apiVersion       = Vulkan_Version,
  };

  // layers
#ifndef NDEBUG
  auto validation_layer = "VK_LAYER_KHRONOS_validation";
  print_supported_instance_layers();
  check_layers_support({ validation_layer });
#endif

  // extensions
  auto extensions = get_instance_extensions();
#ifndef NDEBUG
  print_supported_instance_extensions();
#endif
  check_instance_extensions_support(extensions);
#ifndef NDEBUG
  print_enabled_extensions("instance", extensions);
#endif

  // create instance
  VkInstanceCreateInfo create_info =
  {
    .sType                   = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
#ifndef NDEBUG
    .pNext                   = &debug_messenger_info,
#endif
    .pApplicationInfo        = &app_info,
#ifndef NDEBUG
    .enabledLayerCount       = 1,
    .ppEnabledLayerNames     = &validation_layer,
#endif
    .enabledExtensionCount   = (uint32_t)extensions.size(),
    .ppEnabledExtensionNames = extensions.data(),
  };
  throw_if(vkCreateInstance(&create_info, nullptr, &_instance) != VK_SUCCESS,
           "failed to create vulkan instance!");

  _destructors.push([this] { vkDestroyInstance(_instance, nullptr); });
}

void GraphicsEngine::create_debug_messenger()
{
  auto info = get_debug_messenger_create_info();
  throw_if(vkCreateDebugUtilsMessengerEXT(_instance, &info, nullptr, &_debug_messenger) != VK_SUCCESS,
          "failed to create debug utils messenger extension");

  _destructors.push([this] { vkDestroyDebugUtilsMessengerEXT(_instance, _debug_messenger, nullptr); });
}

void GraphicsEngine::create_surface()
{
  _surface = _window->create_surface(_instance);
  _destructors.push([this] { vkDestroySurfaceKHR(_instance, _surface, nullptr); });
}

void GraphicsEngine::select_physical_device()
{
  auto devices = get_supported_physical_devices(_instance);
  auto devices_score = get_physical_devices_score(devices);
  
  for (const auto& [score, device] : std::ranges::views::reverse(devices_score))
  {
    if (score > 0)
    {
      auto queue_family_indices = get_queue_family_indices(device, _surface);
      if (check_device_extensions_support(device, Device_Extensions) &&
          !get_swapchain_details(device, _surface).has_empty())
      {
        _physical_device = device;
        break;
      }
    }
  }

  throw_if(_physical_device == VK_NULL_HANDLE, "failed to find a suitable GPU");

  auto max_msaa_sample_count = get_max_multisample_count(_physical_device);
  //throw_if(_msaa_sample_count > max_msaa_sample_count, "unsupported 4xmsaa");

#ifndef NDEBUG
  print_supported_physical_devices(_instance);
  print_supported_device_extensions(_physical_device);
  print_supported_max_msaa_sample_count(max_msaa_sample_count);
#endif
}

void GraphicsEngine::create_device_and_get_queues()
{
  auto queue_families = get_queue_family_indices(_physical_device, _surface);
  // if graphic and present family are same index, indices will be one
  std::set<uint32_t> indices
  {
    queue_families.graphics_family.value(),
    queue_families.present_family.value(),
  };

  float priority = 1.0f;

  //
  // create device
  //
  std::vector<VkDeviceQueueCreateInfo> queue_infos;
  for (auto index : indices)
    queue_infos.emplace_back(VkDeviceQueueCreateInfo
    {
      .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
      .queueFamilyIndex = index,
      .queueCount = 1,
      .pQueuePriorities = &priority,
    });

  // features
  VkPhysicalDeviceShaderObjectFeaturesEXT shader_object_features
  {
    .sType        = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_OBJECT_FEATURES_EXT,
    .shaderObject = VK_TRUE,
  };
  VkPhysicalDeviceDescriptorBufferFeaturesEXT descriptor_buffer_features
  {
    .sType            = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_BUFFER_FEATURES_EXT,
    .pNext            = &shader_object_features,
    .descriptorBuffer = VK_TRUE,
  };
  VkPhysicalDeviceVulkan13Features features13
  { 
    .sType               = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES,
    .pNext               = &descriptor_buffer_features, 
    .synchronization2    = VK_TRUE,
    .dynamicRendering    = VK_TRUE,
  };
  VkPhysicalDeviceVulkan12Features features12
  { 
    .sType               = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES,
    .pNext               = &features13,
    .descriptorIndexing  = VK_TRUE,
    .bufferDeviceAddress = VK_TRUE,
  };
  VkPhysicalDeviceFeatures2 features2
  {
    .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2,
    .pNext = &features12,
  };

  // device info 
  VkDeviceCreateInfo create_info
  {
    .sType                   = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
    .pNext                   = &features2,
    .queueCreateInfoCount    = (uint32_t)queue_infos.size(),
    .pQueueCreateInfos       = queue_infos.data(),
    .enabledExtensionCount   = (uint32_t)Device_Extensions.size(),
    .ppEnabledExtensionNames = Device_Extensions.data(),
  };
#ifndef NDEBUG
  print_enabled_extensions("device", Device_Extensions);
#endif

  // create logical device
  _device.init(_physical_device, create_info);

  _destructors.push([this] { _device.destroy(); });

  //
  // get queues
  //
  vkGetDeviceQueue(_device, queue_families.graphics_family.value(), 0, &_graphics_queue);
  vkGetDeviceQueue(_device, queue_families.present_family.value(), 0, &_present_queue);
}
    
void GraphicsEngine::init_memory_allocator()
{
  _mem_alloc.init(_physical_device, _device, _instance, Vulkan_Version);
  _destructors.push([this] { _mem_alloc.destroy(); });
}

void GraphicsEngine::create_swapchain(VkSwapchainKHR old_swapchain)
{
  auto details         = get_swapchain_details(_physical_device, _surface);
  auto surface_format  = details.get_surface_format();
  auto present_mode    = details.get_present_mode();
  auto extent          = details.get_swap_extent(*_window);
  uint32_t image_count = details.capabilities.minImageCount + 1;

  if (details.capabilities.maxImageCount > 0 &&
      image_count > details.capabilities.maxImageCount)
    image_count = details.capabilities.maxImageCount;

  VkSwapchainCreateInfoKHR create_info
  {
    .sType            = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
    .surface          = _surface,
    .minImageCount    = image_count,
    .imageFormat      = surface_format.format,
    .imageColorSpace  = surface_format.colorSpace,
    .imageExtent      = extent,
    .imageArrayLayers = 1,
    .imageUsage       = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT |
                        VK_IMAGE_USAGE_TRANSFER_DST_BIT,
    .preTransform     = details.capabilities.currentTransform,
    .compositeAlpha   = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
    .presentMode      = present_mode,
    .clipped          = VK_TRUE,
    .oldSwapchain     = old_swapchain,
  };

  auto queue_families = get_queue_family_indices(_physical_device, _surface);
  uint32_t indices[]
  {
    queue_families.graphics_family.value(),
    queue_families.present_family.value(),
  };

  if (queue_families.graphics_family != queue_families.present_family)
  {
    create_info.imageSharingMode      = VK_SHARING_MODE_CONCURRENT;
    create_info.queueFamilyIndexCount = 2;
    create_info.pQueueFamilyIndices   = indices;
  }
  else
    create_info.imageSharingMode      = VK_SHARING_MODE_EXCLUSIVE;

  throw_if(vkCreateSwapchainKHR(_device, &create_info, nullptr, &_swapchain) != VK_SUCCESS,
      "failed to create swapchain");

  //
  // get swapchain images info
  //

  // clear old image infos
  if (!_swapchain_images.empty())
  {
    for (auto const& image : _swapchain_images)
      vkDestroyImageView(_device, image.view(), nullptr);
    _swapchain_images.clear();
  }

  // get image count
  vkGetSwapchainImagesKHR(_device, _swapchain, &image_count, nullptr);

  // get swapchain images
  auto images = std::vector<VkImage>(image_count);
  vkGetSwapchainImagesKHR(_device, _swapchain, &image_count, images.data());

  // create image views
  auto image_views = std::vector<VkImageView>(image_count);
  for (auto i = 0; i < image_count; ++i)
  {
    VkImageViewCreateInfo info
    {
      .sType    = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
      .image    = images[i],
      .viewType = VK_IMAGE_VIEW_TYPE_2D,
      .format   = surface_format.format,
      .subresourceRange =
      {
        .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
        .levelCount = 1,
        .layerCount = 1,
      },
    };
    vkCreateImageView(_device, &info, nullptr, &image_views[i]);
  }

  // set image info to _swapchain_images
  _swapchain_images.reserve(image_count);
  for (auto i = 0; i < image_count; ++i)
    _swapchain_images.emplace_back(images[i], image_views[i], extent, surface_format.format);

  if (old_swapchain == VK_NULL_HANDLE)
  {
#ifndef NDEBUG 
    print_present_mode(present_mode);
    std::println("swapchain image counts: {}\n", image_count);
#endif
    _destructors.push([this]
    {
      vkDestroySwapchainKHR(_device, _swapchain, nullptr);
      for (auto const& image : _swapchain_images)
        vkDestroyImageView(_device, image.view(), nullptr);
    });
  }
}

void GraphicsEngine::init_command_pool()
{
  // create command pool
  auto queue_families = get_queue_family_indices(_physical_device, _surface);
  _command_pool.init(_device, queue_families.graphics_family.value());
  _destructors.push([this] { _command_pool.destroy(); });
}

void GraphicsEngine::create_frame_resources()
{
  _frames.resize(_swapchain_images.size());

  // create command buffers
  auto cmd_bufs = _command_pool.create_commands(_swapchain_images.size());
  for (uint32_t i = 0; i < _frames.size(); ++i)
    _frames[i].cmd = std::move(cmd_bufs[i]);

  // async objects 
  VkFenceCreateInfo fence_info
  {
    .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
    .flags = VK_FENCE_CREATE_SIGNALED_BIT,
  };
  VkSemaphoreCreateInfo sem_info
  {
    .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
  };
  for (auto& frame : _frames) 
    throw_if(vkCreateFence(_device, &fence_info, nullptr, &frame.fence)                 != VK_SUCCESS ||
             vkCreateSemaphore(_device, &sem_info, nullptr, &frame.image_available_sem) != VK_SUCCESS || 
             vkCreateSemaphore(_device, &sem_info, nullptr, &frame.render_finished_sem) != VK_SUCCESS,
             "faield to create sync objects");

  _destructors.push([&]
  {
    for (auto& frame : _frames)
    {
      vkDestroyFence(_device, frame.fence, nullptr);
      vkDestroySemaphore(_device, frame.image_available_sem, nullptr);
      vkDestroySemaphore(_device, frame.render_finished_sem, nullptr);
    }
  });
}

void GraphicsEngine::resize_swapchain()
{
  // wait GPU to handle finishing resource
  vkDeviceWaitIdle(_device);
  
  // recreate swapchain
  auto old_swapchain = _swapchain;
  create_swapchain(old_swapchain);
  vkDestroySwapchainKHR(_device, old_swapchain, nullptr);
}

void GraphicsEngine::create_sdf_rendering_resource()
{
  // create shader
  _device.create_shaders(
  {
    { _sdf_vert, VK_SHADER_STAGE_VERTEX_BIT,   "shader/SDF_vert.spv" },
    { _sdf_frag, VK_SHADER_STAGE_FRAGMENT_BIT, "shader/SDF_frag.spv" },
  }, true);

  // create pipeline layout
  _sdf_pipeline_layout = _device.create_pipeline_layout();

  // destroy resources
  _destructors.push([&]
  {
    _sdf_pipeline_layout.destroy();
    _sdf_frag.destroy();
    _sdf_vert.destroy();
  });
}

} }
