#include "GraphicsEngine.hpp"
#include "init-util.hpp"
#include "constant.hpp"

#include <ranges>
#include <set>

namespace tk { namespace graphics_engine { 

GraphicsEngine::GraphicsEngine(Window const& window)
  :_window(window)
{
  // only have single graphics engine
  static bool first = true;
  assert(first);
  if (first)  first = false;

  create_instance();
#ifndef NDEBUG
  create_debug_messenger();
#endif
  create_surface();
  select_physical_device();
  create_device_and_get_queues();
  create_vma_allocator();
  create_swapchain_and_get_swapchain_images_info();
  create_swapchain_image_views();
  create_render_pass();
  create_framebuffers();
  create_descriptor_set_layout();
  create_pipeline();
  create_command_pool();
  create_buffers();
  create_descriptor_pool();
  create_descriptor_sets();
  create_frame_resources();
}

GraphicsEngine::~GraphicsEngine()
{
  vkDeviceWaitIdle(_device);
  for (auto& frame : _frames)
    frame.destructors.clear();
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
  _surface = _window.create_surface(_instance);
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

#ifndef NDEBUG
  print_supported_physical_devices(_instance);
  print_supported_device_extensions(_physical_device);
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
    .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2,
    .pNext = &features12
  };

  // device info 
  VkDeviceCreateInfo create_info
  {
    .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
    .pNext = &features2,
    .queueCreateInfoCount = (uint32_t)queue_infos.size(),
    .pQueueCreateInfos = queue_infos.data(),
    .enabledExtensionCount = (uint32_t)Device_Extensions.size(),
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
    
void GraphicsEngine::create_vma_allocator()
{
  VmaAllocatorCreateInfo alloc_info
  {
    .flags            = VMA_ALLOCATOR_CREATE_EXTERNALLY_SYNCHRONIZED_BIT,
    .physicalDevice   = _physical_device,
    .device           = _device,
    .instance         = _instance,
    .vulkanApiVersion = Vulkan_Version,
  };
  throw_if(vmaCreateAllocator(&alloc_info, &_vma_allocator) != VK_SUCCESS,
           "failed to create Vulkan Memory Allocator");
  _destructors.push([this] { vmaDestroyAllocator(_vma_allocator); });
}

void GraphicsEngine::create_swapchain_and_get_swapchain_images_info()
{
  //
  // create swapchain
  //
  auto details         = get_swapchain_details(_physical_device, _surface);
  auto surface_format  = details.get_surface_format();
  auto present_mode    = details.get_present_mode();
  auto extent          = details.get_swap_extent(_window);
  uint32_t image_count = details.capabilities.minImageCount + 1;
#ifndef NDEBUG 
  print_present_mode(present_mode);
  fmt::print(fg(fmt::color::green), "swapchain image counts: {}\n\n", image_count);
#endif
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

  _destructors.push([this] { vkDestroySwapchainKHR(_device, _swapchain, nullptr); });

  //
  // get swapchain images info
  //
  vkGetSwapchainImagesKHR(_device, _swapchain, &image_count, nullptr);
  _swapchain_images.resize(image_count);
  vkGetSwapchainImagesKHR(_device, _swapchain, &image_count, _swapchain_images.data());
  _swapchain_image_format = surface_format.format;
  _swapchain_image_extent = extent;
}

void GraphicsEngine::create_swapchain_image_views()
{
  _swapchain_image_views.resize(_swapchain_images.size());
  for (uint32_t i = 0; i < _swapchain_images.size(); ++i)
  {
    _swapchain_image_views[i] = create_image_view(_device, _swapchain_images[i], _swapchain_image_format);
    _destructors.push([this, i] { vkDestroyImageView(_device, _swapchain_image_views[i], nullptr); });
  }
}

void GraphicsEngine::create_render_pass()
{
  VkAttachmentDescription color_attachment
  {
    .format         = _swapchain_image_format,
    .samples        = VK_SAMPLE_COUNT_1_BIT,
    .loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR,
    .storeOp        = VK_ATTACHMENT_STORE_OP_STORE,
    .stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
    .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
    .initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED,
    .finalLayout    = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
  };

  VkAttachmentReference attach_reference
  {
    .attachment = 0,
    .layout     = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
  };

  VkSubpassDescription subpass
  {
    .pipelineBindPoint    = VK_PIPELINE_BIND_POINT_GRAPHICS,
    .colorAttachmentCount = 1,
    .pColorAttachments    = &attach_reference,
  };

  VkSubpassDependency dependency 
  {
    .srcSubpass    = VK_SUBPASS_EXTERNAL,
    .dstSubpass    = 0,
    .srcStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
    .dstStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
    .srcAccessMask = 0,
    .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
  };

  VkRenderPassCreateInfo create_info
  {
    .sType           = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
    .attachmentCount = 1,
    .pAttachments    = &color_attachment,
    .subpassCount    = 1,
    .pSubpasses      = &subpass,
    .dependencyCount = 1,
    .pDependencies   = &dependency,
  };

  throw_if(vkCreateRenderPass(_device, &create_info, nullptr, &_render_pass) != VK_SUCCESS,
           "failed to create render pass");

  _destructors.push([this] {   vkDestroyRenderPass(_device, _render_pass, nullptr); });
}

void GraphicsEngine::create_framebuffers()
{
  _framebuffers.resize(_swapchain_images.size());
  for (uint32_t i = 0; i < _swapchain_images.size(); ++i)
  {
    VkFramebufferCreateInfo info
    {
      .sType           = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
      .renderPass      = _render_pass,
      .attachmentCount = 1,
      .pAttachments    = &_swapchain_image_views[i],
      .width           = _swapchain_image_extent.width,
      .height          = _swapchain_image_extent.height,
      .layers          = 1,
    };
    throw_if(vkCreateFramebuffer(_device, &info, nullptr, &_framebuffers[i]) != VK_SUCCESS,
            "failed to create frame buffers");
    _destructors.push([this, i] { vkDestroyFramebuffer(_device, _framebuffers[i], nullptr); });
  }
}

void GraphicsEngine::create_descriptor_set_layout()
{
  std::vector<VkDescriptorSetLayoutBinding> layouts
  {
    {
      .binding         = 0,
      .descriptorType  = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
      .descriptorCount = 1,
      .stageFlags      = VK_SHADER_STAGE_VERTEX_BIT,
    },
    {
      .binding         = 1,
      .descriptorType  = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
      .descriptorCount = 1,
      .stageFlags      = VK_SHADER_STAGE_FRAGMENT_BIT,
    },
  };
  VkDescriptorSetLayoutCreateInfo info
  {
    .sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
    .bindingCount = (uint32_t)layouts.size(),
    .pBindings    = layouts.data(),
  };
  throw_if(vkCreateDescriptorSetLayout(_device, &info, nullptr, &_descriptor_set_layout) != VK_SUCCESS,
           "failed to create descriptor set layout");

  _destructors.push([this] {   vkDestroyDescriptorSetLayout(_device, _descriptor_set_layout, nullptr); });
}

void GraphicsEngine::create_pipeline()
{
  // shader stages
  std::vector<VkPipelineShaderStageCreateInfo> shader_stages;

  Shader vertex_shader(_device, "build/vertex.spv");
  Shader fragment_shader(_device, "build/fragment.spv");

  VkPipelineShaderStageCreateInfo shader_info
  {
    .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
    .stage = VK_SHADER_STAGE_VERTEX_BIT,
    .module = vertex_shader.shader,
    .pName = "main",
  };
  shader_stages.emplace_back(shader_info);

  shader_info.stage  = VK_SHADER_STAGE_FRAGMENT_BIT;
  shader_info.module = fragment_shader.shader;
  shader_stages.emplace_back(shader_info);

  // vertex input info
  auto binding_desc = Vertex::get_binding_description();
  auto attribute_descs = Vertex::get_attribute_descriptions();
  VkPipelineVertexInputStateCreateInfo vertex_input_info
  {
    .sType                           = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
    .vertexBindingDescriptionCount   = 1,
    .pVertexBindingDescriptions      = &binding_desc,
    .vertexAttributeDescriptionCount = (uint32_t)attribute_descs.size(),
    .pVertexAttributeDescriptions    = attribute_descs.data(),
  };

  // input assembly
  VkPipelineInputAssemblyStateCreateInfo input_assembly
  {
    .sType                  = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
    .topology               = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
    .primitiveRestartEnable = VK_FALSE,
  };

  // viewport state
  VkPipelineViewportStateCreateInfo viewport_state
  {
    .sType         = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
    .viewportCount = 1,
    .scissorCount  = 1,
  };

  // rasterization
  VkPipelineRasterizationStateCreateInfo rasterization_state
  {
    .sType                   = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
    .depthClampEnable        = VK_FALSE,   
    .rasterizerDiscardEnable = VK_FALSE,
    .polygonMode             = VK_POLYGON_MODE_FILL,
    // .cullMode                = VK_CULL_MODE_FRONT_BIT,
    .frontFace               = VK_FRONT_FACE_CLOCKWISE,
    .depthBiasEnable         = VK_FALSE,
    .lineWidth               = 1.f,
  };

  // multisample
  VkPipelineMultisampleStateCreateInfo multisample_state
  {
    .sType                = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
    .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
    .sampleShadingEnable  = VK_FALSE,
    .minSampleShading     = 1.f,
  };

  // color blend
  VkPipelineColorBlendAttachmentState color_blend_attachment
  {
    .blendEnable = VK_FALSE,
    .colorWriteMask = VK_COLOR_COMPONENT_R_BIT |
                      VK_COLOR_COMPONENT_G_BIT |
                      VK_COLOR_COMPONENT_B_BIT |
                      VK_COLOR_COMPONENT_A_BIT,
  };
  VkPipelineColorBlendStateCreateInfo color_blend
  {
    .sType           = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
    .logicOpEnable   = VK_FALSE,
    .logicOp         = VK_LOGIC_OP_COPY,
    .attachmentCount = 1,
    .pAttachments    = &color_blend_attachment,
  };

  // dynamic
  std::vector<VkDynamicState> dynamics
  {
    VK_DYNAMIC_STATE_VIEWPORT,
    VK_DYNAMIC_STATE_SCISSOR,
  };
  VkPipelineDynamicStateCreateInfo dynamic
  {
    .sType             = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
    .dynamicStateCount = (uint32_t)dynamics.size(),
    .pDynamicStates    = dynamics.data(),
  };

  // pipeline layout
  VkPipelineLayoutCreateInfo layout_info
  {
    .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
    .setLayoutCount = 1,
    .pSetLayouts    = &_descriptor_set_layout,
  };
  throw_if(vkCreatePipelineLayout(_device, &layout_info, nullptr, &_pipeline_layout) != VK_SUCCESS,
           "failed to create pipeline layout");

  VkGraphicsPipelineCreateInfo create_info
  {
    .sType               = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
    .stageCount          = 2,
    .pStages             = shader_stages.data(),
    .pVertexInputState   = &vertex_input_info,
    .pInputAssemblyState = &input_assembly,
    .pViewportState      = &viewport_state,
    .pRasterizationState = &rasterization_state,
    .pMultisampleState   = &multisample_state,
    .pColorBlendState    = &color_blend,
    .pDynamicState       = &dynamic,
    .layout              = _pipeline_layout,
    // FIX: use dynamic rendering, not need render pass and framebuffers
    .renderPass          = _render_pass,
    .subpass             = 0,
    .basePipelineHandle  = VK_NULL_HANDLE,
    .basePipelineIndex   = -1,
  };

  throw_if(vkCreateGraphicsPipelines(_device, VK_NULL_HANDLE, 1, &create_info, nullptr, &_pipeline) != VK_SUCCESS,
           "failed to create pipeline");

  _destructors.push([this] { vkDestroyPipelineLayout(_device, _pipeline_layout, nullptr); });
  _destructors.push([this] { vkDestroyPipeline(_device, _pipeline, nullptr); });
}

void GraphicsEngine::create_command_pool()
{
  // create command pool
  auto queue_families = get_queue_family_indices(_physical_device, _surface);
  VkCommandPoolCreateInfo command_pool_info
  {
    .sType            = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
    .flags            = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
    .queueFamilyIndex = queue_families.graphics_family.value(),
  };
  throw_if(vkCreateCommandPool(_device, &command_pool_info, nullptr, &_command_pool) != VK_SUCCESS,
           "failed to create command pool");

  _destructors.push([this] {   vkDestroyCommandPool(_device, _command_pool, nullptr); });
}

void GraphicsEngine::create_buffers()
{
  create_buffer(_vertex_buffer, _vertex_buffer_allocation, sizeof(Vertices[0]) * Vertices.size(),VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, Vertices.data());
  create_buffer(_index_buffer, _index_buffer_allocation, sizeof(Indices[0]) * Indices.size(), VK_BUFFER_USAGE_INDEX_BUFFER_BIT, Indices.data());

  _uniform_buffers.resize(Max_Frame_Number);
  _uniform_buffer_allocations.resize(Max_Frame_Number);
  for (int i = 0; i < Max_Frame_Number; ++i)
    create_buffer(_uniform_buffers[i], _uniform_buffer_allocations[i], sizeof(UniformBufferObject), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT);

  for (uint32_t i = 0; i < _uniform_buffers.size(); ++i)
    _destructors.push([this, i] { vmaDestroyBuffer(_vma_allocator, _uniform_buffers[i], _uniform_buffer_allocations[i]); });
  _destructors.push([this] { vmaDestroyBuffer(_vma_allocator, _index_buffer, _index_buffer_allocation); });
  _destructors.push([this] { vmaDestroyBuffer(_vma_allocator, _vertex_buffer, _vertex_buffer_allocation); });
}

void GraphicsEngine::create_descriptor_pool()
{
  std::vector<VkDescriptorPoolSize> sizes
  {
    {
      .type            = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
      .descriptorCount = Max_Frame_Number,
    },
    {
      .type            = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
      .descriptorCount = Max_Frame_Number,
    },
  };
  VkDescriptorPoolCreateInfo info
  {
    .sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
    .maxSets       = Max_Frame_Number,
    .poolSizeCount = (uint32_t)sizes.size(),
    .pPoolSizes    = sizes.data(),
  };
  throw_if(vkCreateDescriptorPool(_device, &info, nullptr, &_descriptor_pool) != VK_SUCCESS,
           "failed to create descriptor pool");
  
  _destructors.push([this] { vkDestroyDescriptorPool(_device, _descriptor_pool, nullptr); });
}

void GraphicsEngine::create_descriptor_sets()
{
  _descriptor_sets.resize(Max_Frame_Number);

  std::vector<VkDescriptorSetLayout> layouts(Max_Frame_Number, _descriptor_set_layout);
  VkDescriptorSetAllocateInfo info
  {
    .sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
    .descriptorPool     = _descriptor_pool,
    .descriptorSetCount = Max_Frame_Number,
    .pSetLayouts        = layouts.data(),
  };
  throw_if(vkAllocateDescriptorSets(_device, &info, _descriptor_sets.data()) != VK_SUCCESS,
           "failed to create descriptor sets");

  for (uint32_t i = 0; i < Max_Frame_Number; ++i)
  {
    VkDescriptorBufferInfo info
    {
      .buffer = _uniform_buffers[i],
      .range  = sizeof(UniformBufferObject),
    };

    std::vector<VkWriteDescriptorSet> writes
    {
      {
        .sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .dstSet          = _descriptor_sets[i],
        .dstBinding      = 0,
        .descriptorCount = 1,
        .descriptorType  = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
        .pBufferInfo     = &info,
      },
    };
    vkUpdateDescriptorSets(_device, writes.size(), writes.data(), 0, nullptr);
  }
}

void GraphicsEngine::create_frame_resources()
{
  _frames.resize(Max_Frame_Number);

  // create command buffers
  auto cmd_bufs = std::vector<VkCommandBuffer>(_frames.size());
  VkCommandBufferAllocateInfo command_buffer_info
  {
    .sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
    .commandPool        = _command_pool,
    .level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
    .commandBufferCount = (uint32_t)_frames.size(),
  };
  throw_if(vkAllocateCommandBuffers(_device, &command_buffer_info, cmd_bufs.data()) != VK_SUCCESS,
           "failed to create command buffers");
  for (uint32_t i = 0; i < _frames.size(); ++i)
    _frames[i].command_buffer = cmd_bufs[i];

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
  {
    throw_if(vkCreateFence(_device, &fence_info, nullptr, &frame.fence)                 != VK_SUCCESS ||
             vkCreateSemaphore(_device, &sem_info, nullptr, &frame.image_available_sem) != VK_SUCCESS || 
             vkCreateSemaphore(_device, &sem_info, nullptr, &frame.render_finished_sem) != VK_SUCCESS,
             "faield to create sync objects");

    _destructors.push([&]
    {
      vkDestroyFence(_device, frame.fence, nullptr);
      vkDestroySemaphore(_device, frame.image_available_sem, nullptr);
      vkDestroySemaphore(_device, frame.render_finished_sem, nullptr);
    });
  }
}

} }
