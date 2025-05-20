#include "tk/GraphicsEngine/GraphicsEngine.hpp"
#include "init-util.hpp"
#include "tk/GraphicsEngine/Shader.hpp"
#include "tk/GraphicsEngine/vk_extension.hpp"

#include <SMAA/AreaTex.h>
#include <SMAA/SearchTex.h>

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
  load_instance_extension_funcs();
#ifndef NDEBUG
  create_debug_messenger();
#endif
  create_surface();
  select_physical_device();
  create_device_and_get_queues();
  load_device_extension_funcs();
  init_memory_allocator();
  init_command_pool();
  create_swapchain_and_rendering_image();
  load_precalculated_textures();
  create_descriptor_set_layout();
  create_shaders_and_pipeline_layouts();
  //create_graphics_pipeline();
  create_buffer();
  create_frame_resources();

  create_sdfaa_resources();
  
  update_descriptors_to_descrptor_buffer();

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
  //VkPhysicalDeviceGraphicsPipelineLibraryFeaturesEXT graphics_library_features
  //{
  //  .sType                   = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_GRAPHICS_PIPELINE_LIBRARY_FEATURES_EXT,
  //  .pNext                   = nullptr,
  //  .graphicsPipelineLibrary = VK_TRUE,
  //};
  VkPhysicalDeviceShaderObjectFeaturesEXT shader_object_features
  {
    .sType        = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_OBJECT_FEATURES_EXT,
    //.pNext        = &graphics_library_features,
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
  // MSAA + SMAA
  //
  //_msaa_image     = _mem_alloc.create_image(_swapchain_images[0].format, _swapchain_images[0].extent, VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, _msaa_sample_count);
  _resolved_image = _mem_alloc.create_image(_swapchain_images[0].format, _swapchain_images[0].extent, VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);
  // FIXME: tmp transform src
  _edges_image    = _mem_alloc.create_image(VK_FORMAT_R8G8_UNORM, _swapchain_images[0].extent,        VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT);
  _blend_image    = _mem_alloc.create_image(VK_FORMAT_R8G8B8A8_UNORM, _swapchain_images[0].extent,    VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT);
  _smaa_image     = _mem_alloc.create_image(_swapchain_images[0].format, _swapchain_images[0].extent, VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT);
  // create msaa depth image and depth image
  // _msaa_depth_image.format = Depth_Format;
  // _msaa_depth_image.extent = _image.extent;
  // VkImageCreateInfo msaa_depth_info
  // {
  //   .sType       = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
  //   .imageType   = VK_IMAGE_TYPE_2D,
  //   .format      = _msaa_depth_image.format,
  //   .extent      = _msaa_depth_image.extent,
  //   .mipLevels   = 1,
  //   .arrayLayers = 1,
  //   .samples     = _msaa_sample_count,
  //   .tiling      = VK_IMAGE_TILING_OPTIMAL,
  //   .usage       = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT |
  //                  VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT,
  // };
  // throw_if(vmaCreateImage(_mem_alloc.get(), &msaa_depth_info, &alloc_info, &_msaa_depth_image.image, &_msaa_depth_image.allocation, nullptr) != VK_SUCCESS,
  //          "failed to create msaa depth image");
  // auto depth_info = msaa_depth_info;
  // depth_info.samples = VK_SAMPLE_COUNT_1_BIT;
  // for (auto i = 0; i < _frames.size(); ++i)
  //   throw_if(vmaCreateImage(_mem_alloc.get(), &depth_info, &alloc_info, &_frames[i].depth_image.image, &_frames[i].depth_image.allocation, nullptr) != VK_SUCCESS,
  //            "failed to create depth image");

  // VkImageViewCreateInfo msaa_depth_view_info
  // {
  //   .sType    = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
  //   .image    = _msaa_depth_image.image,
  //   .viewType = VK_IMAGE_VIEW_TYPE_2D,
  //   .format   = _msaa_depth_image.format,
  //   .subresourceRange =
  //   {
  //     .aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT,
  //     .levelCount = 1,
  //     .layerCount = 1,
  //   },
  // };
  // throw_if(vkCreateImageView(_device, &msaa_depth_view_info, nullptr, &_msaa_depth_image.view) != VK_SUCCESS,
  //          "failed to create msaa depth image view");
  // auto depth_view_info = msaa_depth_view_info;
  // for (auto i = 0; i < _frames.size(); ++i)
  // {
  //   depth_view_info.image = _frames[i].depth_image.image;
  //   throw_if(vkCreateImageView(_device, &depth_view_info, nullptr, &_frames[i].depth_image.view) != VK_SUCCESS,
  //            "failed to create depth image view");
  // }

  _destructors.push([this]
  {
    // for (auto i = 0; i < _frames.size(); ++i)
    // {
    //   vkDestroyImageView(_device, _frames[i].depth_image.view, nullptr);
    //   vmaDestroyImage(_mem_alloc.get(), _frames[i].depth_image.image, _frames[i].depth_image.allocation);
    // }
    // vkDestroyImageView(_device, _msaa_depth_image.view, nullptr);
    // vmaDestroyImage(_mem_alloc.get(), _msaa_depth_image.image, _msaa_depth_image.allocation);
    _mem_alloc.destroy_image(_blend_image);
    _mem_alloc.destroy_image(_edges_image);
    _mem_alloc.destroy_image(_resolved_image);
    //_mem_alloc.destroy_image(_msaa_image);
    _mem_alloc.destroy_image(_smaa_image);
    vkDestroySwapchainKHR(_device, _swapchain, nullptr);
    for (auto const& image : _swapchain_images)
      vkDestroyImageView(_device, image.view, nullptr);
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

  // clear old image infos
  if (!_swapchain_images.empty())
  {
    for (auto const& image : _swapchain_images)
      vkDestroyImageView(_device, image.view, nullptr);
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
  {
    _swapchain_images.emplace_back(Image
    {
      .handle = images[i],
      .view   = image_views[i],
      .extent = { extent.width, extent.height, 1 },
      .format = surface_format.format,
    });
  }
}

void GraphicsEngine::create_descriptor_set_layout()
{
  _smaa_descriptor_layout = _device.create_descriptor_layout(
  {
    { 0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_COMPUTE_BIT, _resolved_image.view, _smaa_sampler },
    { 1, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,          VK_SHADER_STAGE_COMPUTE_BIT, _edges_image.view                   },
    { 2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_COMPUTE_BIT, _edges_image.view,    _smaa_sampler },
    { 3, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_COMPUTE_BIT, _area_texture.view,   _smaa_sampler },
    { 4, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_COMPUTE_BIT, _search_texture.view, _smaa_sampler },
    { 5, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,          VK_SHADER_STAGE_COMPUTE_BIT, _blend_image.view                   },
    { 6, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_COMPUTE_BIT, _blend_image.view,    _smaa_sampler },
    { 7, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,          VK_SHADER_STAGE_COMPUTE_BIT, _smaa_image.view                    },
  });

  _destructors.push([this] { _smaa_descriptor_layout.destroy(); });
}

void GraphicsEngine::create_shaders_and_pipeline_layouts()
{
  // push constant
  VkPushConstantRange push_constant_range 
  {
    .stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
    .size       = sizeof(PushConstant),
  };
  auto smaa_push_constant = VkPushConstantRange
  {
    .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
    .size       = sizeof(PushConstant_SMAA),
  };

  // create shaders
  _device.create_shaders(
  { 
    { _2D_vert,                  VK_SHADER_STAGE_VERTEX_BIT,   "shader/2D_vert.spv",                  {},                          { push_constant_range } },
    { _2D_frag,                  VK_SHADER_STAGE_FRAGMENT_BIT, "shader/2D_frag.spv",                  {},                          { push_constant_range } },
  }, true);
  _device.create_shaders(
  {
    { _SMAA_edge_detection_comp, VK_SHADER_STAGE_COMPUTE_BIT,  "shader/SMAA_edge_detection_comp.spv", { _smaa_descriptor_layout }, { smaa_push_constant } },
    { _SMAA_blend_weight_comp,   VK_SHADER_STAGE_COMPUTE_BIT,  "shader/SMAA_blend_weight_comp.spv",   { _smaa_descriptor_layout }, { smaa_push_constant } },
    { _SMAA_neighbor_comp,       VK_SHADER_STAGE_COMPUTE_BIT,  "shader/SMAA_neighbor_comp.spv",       { _smaa_descriptor_layout }, { smaa_push_constant } },
  });

  // create pipeline layouts
  _2D_pipeline_layout   = _device.create_pipeline_layout({},                          { push_constant_range });
  _smaa_pipeline_layout = _device.create_pipeline_layout({ _smaa_descriptor_layout }, { smaa_push_constant  });

  _destructors.push([this]
  {
    _2D_vert.destroy();
    _2D_frag.destroy();
    _SMAA_edge_detection_comp.destroy();
    _SMAA_blend_weight_comp.destroy();
    _SMAA_neighbor_comp.destroy();
    _2D_pipeline_layout.destroy();
    _smaa_pipeline_layout.destroy();
  });
}

/*
void GraphicsEngine::create_graphics_pipeline()
{
  // create 2D pipeline
  VkPushConstantRange push_constant_range 
  {
    .stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
    .size       = sizeof(PushConstant),
  };
  auto smaa_push_constant = VkPushConstantRange
  {
    .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
    .size       = sizeof(PushConstant_SMAA),
  };
  VkPipelineLayoutCreateInfo layout_info
  {
    .sType                  = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
    .pushConstantRangeCount = 1,
    .pPushConstantRanges    = &push_constant_range,
  };
  throw_if(vkCreatePipelineLayout(_device, &layout_info, nullptr, &_2D_pipeline_layout) != VK_SUCCESS,
           "failed to create graphics pipeline layout");

  _destructors.push([this]
  {
    _2D_vert.destroy();
    _2D_frag.destroy();
    _SMAA_edge_detection_comp.destroy();
    _SMAA_blend_weight_comp.destroy();
    _SMAA_neighbor_comp.destroy();
  });

  auto builder = Pipeline();

  auto shader_vertex2D   = Pipeline::Shader(_device, "shader/2D_vert.spv");
  auto shader_fragment2D = Pipeline::Shader(_device, "shader/2D_frag.spv");
  _2D_pipeline = builder
                 .clear()
                 .set_shaders(shader_vertex2D.shader, shader_fragment2D.shader)
                 // TODO: imgui use counter clockwise
                 .set_cull_mode(VK_CULL_MODE_BACK_BIT, VK_FRONT_FACE_CLOCKWISE)
                 .set_color_attachment_format(_swapchain_images[0].format)
                 // TODO: imgui not enable depth test
                 //.enable_depth_test(Depth_Format)
                 .set_msaa(VK_SAMPLE_COUNT_1_BIT)
                 .build(_device, _2D_pipeline_layout);

  // smaa pipelines
  push_constant_range.stageFlags |= VK_SHADER_STAGE_FRAGMENT_BIT;
  push_constant_range.size = sizeof(PushConstant_SMAA);
  layout_info.setLayoutCount = 1;
  layout_info.pSetLayouts = _smaa_descriptor_layout.get_address();
  throw_if(vkCreatePipelineLayout(_device, &layout_info, nullptr, &_smaa_pipeline_layout) != VK_SUCCESS,
           "failed to create smaa edge detection pipeline layout");

  // TODO: use graphics_pipeline_library can not create descriptor set layout
  _smaa_pipeline[0] = _device.create_pipeline(type::pipeline::compute, { "shader/SMAA_edge_detection_comp.spv" }, { _smaa_descriptor_layout }, { smaa_push_constant });
  _smaa_pipeline[1] = _device.create_pipeline(type::pipeline::compute, { "shader/SMAA_blend_weight_comp.spv" }, { _smaa_descriptor_layout }, { smaa_push_constant });
  _smaa_pipeline[2] = _device.create_pipeline(type::pipeline::compute, { "shader/SMAA_neighbor_comp.spv" }, { _smaa_descriptor_layout }, { smaa_push_constant });

  // set destructors
  _destructors.push([this]
  { 
    _smaa_pipeline[2].destroy();
    _smaa_pipeline[1].destroy();
    _smaa_pipeline[0].destroy();
    vkDestroyPipelineLayout(_device, _smaa_pipeline_layout, nullptr);
    vkDestroyPipeline(_device, _2D_pipeline, nullptr);
    vkDestroyPipelineLayout(_device, _2D_pipeline_layout, nullptr);
  });
}
*/

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

  // destroy old images
  //_mem_alloc.destroy_image(_msaa_image);
  _mem_alloc.destroy_image(_resolved_image);
  _mem_alloc.destroy_image(_blend_image);
  _mem_alloc.destroy_image(_edges_image);
  _mem_alloc.destroy_image(_smaa_image);

  // recreate images
  //_msaa_image     = _mem_alloc.create_image(_swapchain_images[0].format, _swapchain_images[0].extent, VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, _msaa_sample_count);
  _resolved_image = _mem_alloc.create_image(_swapchain_images[0].format, _swapchain_images[0].extent, VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);
  _edges_image    = _mem_alloc.create_image(VK_FORMAT_R8G8_UNORM, _swapchain_images[0].extent,        VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT);
  _blend_image    = _mem_alloc.create_image(VK_FORMAT_R8G8B8A8_UNORM, _swapchain_images[0].extent,    VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT);
  _smaa_image     = _mem_alloc.create_image(_swapchain_images[0].format, _swapchain_images[0].extent, VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT);

  // update image views
  _smaa_descriptor_layout.update_descriptor_image_views(
  {
    { 0, _resolved_image.view },
    { 1, _edges_image.view    },
    { 2, _edges_image.view    },
    { 3, _area_texture.view   },
    { 4, _search_texture.view },
    { 5, _blend_image.view    },
    { 6, _blend_image.view    },
    { 7, _smaa_image.view     },
  });
  _smaa_descriptor_layout.update_descriptors(_descriptor_buffer);
}

void GraphicsEngine::create_buffer()
{
  // promise every swapchain image have seperate vertices indices buffer avoid GPU-CPU data race
  // e.g. GPU read last frame unfinished but CPU already write the using buffer next frame data
  _buffer = _mem_alloc.create_buffer(Buffer_Size * _swapchain_images.size(), VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT |
                                                                             VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
                                                                             VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT);
  _destructors.push([this] 
  {
    _buffer.destroy(); 
  });
}

// TODO: abstract load array data to image as a method function of Image
void GraphicsEngine::load_precalculated_textures()
{
  // area texture and search texture (for SMAA)
  _area_texture   = _mem_alloc.create_image(VK_FORMAT_R8G8_UNORM, { AREATEX_WIDTH, AREATEX_HEIGHT, 1 }, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);
  _search_texture = _mem_alloc.create_image(VK_FORMAT_R8_UNORM, { SEARCHTEX_WIDTH, SEARCHTEX_HEIGHT, 1 }, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);
  
  // upload buffer
  auto buf = _mem_alloc.create_buffer(AREATEX_SIZE + SEARCHTEX_SIZE, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT);
  
  // copy texture data
  throw_if(vmaCopyMemoryToAllocation(_mem_alloc.get(), areaTexBytes, buf.allocation(), 0, AREATEX_SIZE) != VK_SUCCESS,
           "failed to copy area texture data to buffer");
  throw_if(vmaCopyMemoryToAllocation(_mem_alloc.get(), searchTexBytes, buf.allocation(), AREATEX_SIZE, SEARCHTEX_SIZE) != VK_SUCCESS,
           "failed to copy search texture data to buffer");
  
  // upload data to textures
  auto cmd = _command_pool.create_command().begin();

  transition_image_layout(cmd, _area_texture.handle, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
  transition_image_layout(cmd, _search_texture.handle, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

  VkBufferImageCopy2 region
  {
    .sType            = VK_STRUCTURE_TYPE_BUFFER_IMAGE_COPY_2,
    .bufferOffset     = 0,
    .imageSubresource =
    {
      .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
      .layerCount = 1,
    },
    .imageExtent = 
    {
      .width  = AREATEX_WIDTH,
      .height = AREATEX_HEIGHT,
      .depth  = 1,
    },
  };

  VkCopyBufferToImageInfo2 info
  {
    .sType          = VK_STRUCTURE_TYPE_COPY_BUFFER_TO_IMAGE_INFO_2,
    .srcBuffer      = buf.handle(),
    .dstImage       = _area_texture.handle,
    .dstImageLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
    .regionCount    = 1,
    .pRegions       = &region,
  };

  vkCmdCopyBufferToImage2(cmd, &info);

  region =
  {
    .sType            = VK_STRUCTURE_TYPE_BUFFER_IMAGE_COPY_2,
    .bufferOffset     = AREATEX_SIZE,
    .imageSubresource =
    {
      .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
      .layerCount = 1,
    },
    .imageExtent = 
    {
      .width  = SEARCHTEX_WIDTH,
      .height = SEARCHTEX_HEIGHT,
      .depth  = 1,
    },
  };
  info.dstImage = _search_texture.handle;
  vkCmdCopyBufferToImage2(cmd, &info);

  transition_image_layout(cmd, _area_texture.handle, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
  transition_image_layout(cmd, _search_texture.handle, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

  cmd.end().submit_wait_free(_command_pool, _graphics_queue);

  // destroy upload buffer
  buf.destroy();

  // sampler
  VkSamplerCreateInfo sampler_info
  {
    .sType        = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
    .magFilter    = VK_FILTER_LINEAR,
    .minFilter    = VK_FILTER_LINEAR,
    .mipmapMode   = VK_SAMPLER_MIPMAP_MODE_LINEAR,
    .addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
    .addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
    .addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
  };
  throw_if(vkCreateSampler(_device, &sampler_info, nullptr, &_smaa_sampler) != VK_SUCCESS,
           "failed to create smaa sampler");

  // record destructors
  _destructors.push([this]
  {
    vkDestroySampler(_device, _smaa_sampler, nullptr);
    _mem_alloc.destroy_image(_area_texture);
    _mem_alloc.destroy_image(_search_texture);
  });
}

void GraphicsEngine::update_descriptors_to_descrptor_buffer()
{
  _descriptor_buffer = _mem_alloc.create_buffer(_smaa_descriptor_layout.size(), VK_BUFFER_USAGE_RESOURCE_DESCRIPTOR_BUFFER_BIT_EXT  | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_SAMPLER_DESCRIPTOR_BUFFER_BIT_EXT , VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT);

  _smaa_descriptor_layout.update_descriptors(_descriptor_buffer);
  
  _destructors.push([this] { _descriptor_buffer.destroy(); });
}

// TODO: need handle recreate swapchain image recreate and descriptor layout update
void GraphicsEngine::create_sdfaa_resources()
{
  // create image
  _sdfaa_image = _mem_alloc.create_image(_swapchain_images[0].format, _swapchain_images[0].extent, VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_STORAGE_BIT);

  // create descriptor layout
  _sdfaa_descriptor_layout = _device.create_descriptor_layout(
  {
    { 0, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT, _sdfaa_image.view },
  });

  // create and upload data of decriptor layout to descriptor buffer
  _sdfaa_descriptor_buffer = _mem_alloc.create_buffer(_sdfaa_descriptor_layout.size(), VK_BUFFER_USAGE_RESOURCE_DESCRIPTOR_BUFFER_BIT_EXT  | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT, VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT);
  _sdfaa_descriptor_layout.update_descriptors(_sdfaa_descriptor_buffer);

  // create shader
  _device.create_shaders(
  {
    { _sdfaa_comp, VK_SHADER_STAGE_COMPUTE_BIT,  "shader/SDFAA_comp.spv", { _sdfaa_descriptor_layout }, {} },
  });

  // create pipeline layout
  _sdfaa_pipeline_layout = _device.create_pipeline_layout({ _sdfaa_descriptor_layout }, {});

  // destroy resources
  _destructors.push([&]
  {
    _sdfaa_pipeline_layout.destroy();
    _sdfaa_comp.destroy();
    _sdfaa_descriptor_buffer.destroy();
    _sdfaa_descriptor_layout.destroy();
    // TODO: change api, use image.destroy();
    _mem_alloc.destroy_image(_sdfaa_image);
  });
}

} }
