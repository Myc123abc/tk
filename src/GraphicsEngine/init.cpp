#include "tk/GraphicsEngine/GraphicsEngine.hpp"
#include "init-util.hpp"
#include "constant.hpp"
#include "tk/GraphicsEngine/PipelineBuilder.hpp"
#include "tk/GraphicsEngine/MaterialLibrary.hpp"

#include <ranges>
#include <set>
#include <print>

namespace tk { namespace graphics_engine { 

void GraphicsEngine::init(Window& window)
{
  // only have single graphics engine
  static bool first = true;
  assert(first);
  if (first)  first = false;

  _window = &window;

  create_instance();
#ifndef NDEBUG
  create_debug_messenger();
#endif
  create_surface();
  select_physical_device();
  create_device_and_get_queues();
  init_memory_allocator();
  init_command_pool();
  create_frame_resources();
  create_swapchain_and_rendering_image();
  create_descriptor_set_layout();
  create_graphics_pipeline();
  create_descriptor_pool();
  create_descriptor_sets();
  use_single_time_command_init_something();

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
  throw_if(graphics_engine::vkCreateDebugUtilsMessengerEXT(_instance, &info, nullptr, &_debug_messenger) != VK_SUCCESS,
          "failed to create debug utils messenger extension");

  _destructors.push([this] { graphics_engine::vkDestroyDebugUtilsMessengerEXT(_instance, _debug_messenger, nullptr); });
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
  throw_if(_msaa_sample_count > max_msaa_sample_count, "unsupported 4xmsaa");

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
  VkPhysicalDeviceVulkan13Features features13
  { 
    .sType               = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES,
    .pNext               = nullptr, 
    .synchronization2    = true,
    .dynamicRendering    = true,
  };
  VkPhysicalDeviceVulkan12Features features12
  { 
    .sType               = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES,
    .pNext               = &features13,
    .descriptorIndexing  = true,
    .bufferDeviceAddress = true,
  };
  VkPhysicalDeviceFeatures2 features2
  {
    .sType    = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2,
    .pNext    = &features12,
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
  throw_if(vkCreateDevice(_physical_device, &create_info, nullptr, &_device) != VK_SUCCESS,
           "failed to create logical device");

  _destructors.push([this] { vkDestroyDevice(_device, nullptr); });

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

void GraphicsEngine::create_swapchain_and_rendering_image()
{
  //
  // create swapchain
  //
  auto details         = get_swapchain_details(_physical_device, _surface);
  auto present_mode    = details.get_present_mode();
  auto image_count     = details.capabilities.minImageCount + 1;
  uint32_t w;
  uint32_t h;
  _window->get_screen_size(w, h);
  auto extent          = VkExtent2D{ w, h, };

#ifndef NDEBUG 
  print_present_mode(present_mode);
  std::println("swapchain image counts: {}\n", image_count);
#endif
  create_swapchain();

  //
  // dynamic rendering use image
  //
  _image.extent = { extent.width, extent.height, 1 };
  _image.format = VK_FORMAT_R16G16B16A16_SFLOAT;
  _msaa_image.extent = _image.extent;
  _msaa_image.format = _image.format;
  
  // INFO: image as copy src to swapchain image
  //       and resolved image from msaa image
  VkImageCreateInfo image_info
  {
    .sType       = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
    .imageType   = VK_IMAGE_TYPE_2D,
    .format      = _image.format,
    .extent      = _image.extent,
    .mipLevels   = 1,
    .arrayLayers = 1,
    .samples     = VK_SAMPLE_COUNT_1_BIT,
    .tiling      = VK_IMAGE_TILING_OPTIMAL,
    .usage       = VK_IMAGE_USAGE_TRANSFER_SRC_BIT     |
                   VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
  };

  auto msaa_image_info = image_info;
  msaa_image_info.samples = _msaa_sample_count;
  msaa_image_info.usage   = VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT |
                            VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

  VmaAllocationCreateInfo alloc_info
  {
    .flags         = VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT,
    .usage         = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE,
    .requiredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
  };
  throw_if(vmaCreateImage(_mem_alloc.get(), &image_info, &alloc_info, &_image.image, &_image.allocation, nullptr) != VK_SUCCESS,
           "failed to create image");
  throw_if(vmaCreateImage(_mem_alloc.get(), &msaa_image_info, &alloc_info, &_msaa_image.image, &_msaa_image.allocation, nullptr) != VK_SUCCESS,
           "failed to create msaa image");

  VkImageViewCreateInfo image_view_info
  {
    .sType    = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
    .image    = _image.image,
    .viewType = VK_IMAGE_VIEW_TYPE_2D,
    .format   = _image.format,
    .subresourceRange =
    {
      .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
      .levelCount = 1,
      .layerCount = 1,
    },
  };
  auto msaa_image_view_info  = image_view_info;
  msaa_image_view_info.image = _msaa_image.image;
  throw_if(vkCreateImageView(_device, &image_view_info, nullptr, &_image.view) != VK_SUCCESS,
           "failed to create image view");
  throw_if(vkCreateImageView(_device, &msaa_image_view_info, nullptr, &_msaa_image.view) != VK_SUCCESS,
           "failed to create msaa image view");

  // create msaa depth image and depth image
  _msaa_depth_image.format = Depth_Format;
  _msaa_depth_image.extent = _image.extent;
  VkImageCreateInfo msaa_depth_info
  {
    .sType       = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
    .imageType   = VK_IMAGE_TYPE_2D,
    .format      = _msaa_depth_image.format,
    .extent      = _msaa_depth_image.extent,
    .mipLevels   = 1,
    .arrayLayers = 1,
    .samples     = _msaa_sample_count,
    .tiling      = VK_IMAGE_TILING_OPTIMAL,
    .usage       = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT |
                   VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT,
  };
  throw_if(vmaCreateImage(_mem_alloc.get(), &msaa_depth_info, &alloc_info, &_msaa_depth_image.image, &_msaa_depth_image.allocation, nullptr) != VK_SUCCESS,
           "failed to create msaa depth image");
  auto depth_info = msaa_depth_info;
  depth_info.samples = VK_SAMPLE_COUNT_1_BIT;
  for (auto i = 0; i < _frames.size(); ++i)
    throw_if(vmaCreateImage(_mem_alloc.get(), &depth_info, &alloc_info, &_frames[i].depth_image.image, &_frames[i].depth_image.allocation, nullptr) != VK_SUCCESS,
             "failed to create depth image");

  VkImageViewCreateInfo msaa_depth_view_info
  {
    .sType    = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
    .image    = _msaa_depth_image.image,
    .viewType = VK_IMAGE_VIEW_TYPE_2D,
    .format   = _msaa_depth_image.format,
    .subresourceRange =
    {
      .aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT,
      .levelCount = 1,
      .layerCount = 1,
    },
  };
  throw_if(vkCreateImageView(_device, &msaa_depth_view_info, nullptr, &_msaa_depth_image.view) != VK_SUCCESS,
           "failed to create msaa depth image view");
  auto depth_view_info = msaa_depth_view_info;
  for (auto i = 0; i < _frames.size(); ++i)
  {
    depth_view_info.image = _frames[i].depth_image.image;
    throw_if(vkCreateImageView(_device, &depth_view_info, nullptr, &_frames[i].depth_image.view) != VK_SUCCESS,
             "failed to create depth image view");
  }

  _destructors.push([this]
  {
    for (auto i = 0; i < _frames.size(); ++i)
    {
      vkDestroyImageView(_device, _frames[i].depth_image.view, nullptr);
      vmaDestroyImage(_mem_alloc.get(), _frames[i].depth_image.image, _frames[i].depth_image.allocation);
    }
    vkDestroyImageView(_device, _msaa_depth_image.view, nullptr);
    vmaDestroyImage(_mem_alloc.get(), _msaa_depth_image.image, _msaa_depth_image.allocation);
    vkDestroyImageView(_device, _msaa_image.view, nullptr);
    vmaDestroyImage(_mem_alloc.get(), _msaa_image.image, _msaa_image.allocation);
    vkDestroyImageView(_device, _image.view, nullptr);
    vmaDestroyImage(_mem_alloc.get(), _image.image, _image.allocation);
    vkDestroySwapchainKHR(_device, _swapchain, nullptr);
  });
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
  vkGetSwapchainImagesKHR(_device, _swapchain, &image_count, nullptr);
  _swapchain_images.resize(image_count);
  vkGetSwapchainImagesKHR(_device, _swapchain, &image_count, _swapchain_images.data());
  _swapchain_image_extent = extent;
}

void GraphicsEngine::create_descriptor_set_layout()
{
  // std::vector<VkDescriptorSetLayoutBinding> layouts
  // {
  //   {
  //     .binding         = 0,
  //     .descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
  //     .descriptorCount = 1,
  //     .stageFlags      = VK_SHADER_STAGE_COMPUTE_BIT,
  //   },
  // };
  // VkDescriptorSetLayoutCreateInfo info
  // {
  //   .sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
  //   .bindingCount = (uint32_t)layouts.size(),
  //   .pBindings    = layouts.data(),
  // };
  // throw_if(vkCreateDescriptorSetLayout(_device, &info, nullptr, &_descriptor_set_layout) != VK_SUCCESS,
  //          "failed to create descriptor set layout");
  //
  // _destructors.push([this] { vkDestroyDescriptorSetLayout(_device, _descriptor_set_layout, nullptr); });
}

void GraphicsEngine::create_graphics_pipeline()
{
  // create pipeline layout
  VkPushConstantRange push_constant_range 
  {
    .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
    .size       = sizeof(PushConstant),
  };
  VkPipelineLayoutCreateInfo layout_info
  {
    .sType                  = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
    .pushConstantRangeCount = 1,
    .pPushConstantRanges    = &push_constant_range,
  };
  throw_if(vkCreatePipelineLayout(_device, &layout_info, nullptr, &_2D_pipeline_layout) != VK_SUCCESS,
           "failed to create graphics pipeline layout");

  // create pipeline
  auto shader_vertex2D   = Shader(_device, "build/2D_vert.spv");
  auto shader_fragment2D = Shader(_device, "build/2D_frag.spv");
  _2D_pipeline = PipelineBuilder()
                 .clear()
                 .set_shaders(shader_vertex2D.shader, shader_fragment2D.shader)
                 // TODO: imgui use counter clockwise
                 .set_cull_mode(VK_CULL_MODE_BACK_BIT, VK_FRONT_FACE_CLOCKWISE)
                 .set_color_attachment_format(_image.format)
                 // TODO: imgui not enable depth test
                 //.enable_depth_test(Depth_Format)
                 // TODO: imgui not use msaa
                 .set_msaa(_msaa_sample_count)
                 .build(_device, _2D_pipeline_layout);

  // set destructors
  _destructors.push([this]
  { 
    vkDestroyPipeline(_device, _2D_pipeline, nullptr);
    vkDestroyPipelineLayout(_device, _2D_pipeline_layout, nullptr);
  });
}

void GraphicsEngine::init_command_pool()
{
  // create command pool
  auto queue_families = get_queue_family_indices(_physical_device, _surface);
  _command_pool.init(_device, queue_families.graphics_family.value());
  _destructors.push([this] { _command_pool.destroy(); });
}

void GraphicsEngine::create_descriptor_pool()
{
  // std::vector<VkDescriptorPoolSize> sizes
  // {
  //   {
  //     .type            = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
  //     .descriptorCount = 1,
  //   },
  // };
  // VkDescriptorPoolCreateInfo info
  // {
  //   .sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
  //   .maxSets       = 1,
  //   .poolSizeCount = (uint32_t)sizes.size(),
  //   .pPoolSizes    = sizes.data(),
  // };
  // throw_if(vkCreateDescriptorPool(_device, &info, nullptr, &_descriptor_pool) != VK_SUCCESS,
  //          "failed to create descriptor pool");
  // 
  // _destructors.push([this] { vkDestroyDescriptorPool(_device, _descriptor_pool, nullptr); });
}

void GraphicsEngine::create_descriptor_sets()
{
  // VkDescriptorSetAllocateInfo info
  // {
  //   .sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
  //   .descriptorPool     = _descriptor_pool,
  //   .descriptorSetCount = 1,
  //   .pSetLayouts        = &_descriptor_set_layout,
  // };
  // throw_if(vkAllocateDescriptorSets(_device, &info, &_descriptor_set) != VK_SUCCESS,
  //          "failed to create descriptor set");
  //
  // VkDescriptorImageInfo img_info
  // {
  //   .imageView   = _image.view,
  //   .imageLayout = VK_IMAGE_LAYOUT_GENERAL,
  // };
  //
  // VkWriteDescriptorSet write
  // {
  //   .sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
  //   .dstSet          = _descriptor_set,
  //   .dstBinding      = 0,
  //   .descriptorCount = 1,
  //   .descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
  //   .pImageInfo     = &img_info,
  // };
  //
  // vkUpdateDescriptorSets(_device, 1, &write, 0, nullptr);
}

void GraphicsEngine::create_frame_resources()
{
  _frames.resize(Max_Frame_Number);

  // create command buffers
  auto cmd_bufs = _command_pool.create_commands(Max_Frame_Number);
  for (uint32_t i = 0; i < _frames.size(); ++i)
    _frames[i].command_buffer = std::move(cmd_bufs[i]);

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
  vkDeviceWaitIdle(_device);
  auto old_swapchain = _swapchain;
  create_swapchain(old_swapchain);
  vkDestroySwapchainKHR(_device, old_swapchain, nullptr);
}

void GraphicsEngine::use_single_time_command_init_something()
{
  // start single time command and destructor use for destruct stage buffer, etc.
  auto cmd        = _command_pool.create_command().begin();
  auto destructor = DestructorStack();

  // use command
  MaterialLibrary::init(_mem_alloc, cmd, destructor);

  // end command
  cmd.end().submit_wait_free(_command_pool, _graphics_queue);

  // destroy stage buffer and others
  destructor.clear();

  // add material library destructor
  _destructors.push([&] { MaterialLibrary::destroy(); });
}

} }
