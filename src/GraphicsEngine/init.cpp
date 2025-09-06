#include "GraphicsEngine.hpp"
#include "init-util.hpp"
#include "vk_extension.hpp"
#include "config.hpp"
#include "Features.hpp"

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
  create_swapchain();
  init_command_pool();
  init_memory_allocator();
  create_sampler();

  create_frame_resources();

  init_text_engine();
  init_sdf_resources();
  
  init_gpu_resource();
}

void GraphicsEngine::destroy()
{
  if (_device)
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
    .apiVersion       = config()->vulkan_version,
  };

  // layers
#ifndef NDEBUG
  auto validation_layer = "VK_LAYER_KHRONOS_validation";
  check_layers_support({ validation_layer });
#endif

  // extensions
  auto extensions = get_instance_extensions();
  check_instance_extensions_support(extensions);

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
  _surface = _window->create_vulkan_surface(_instance);
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
      if (check_device_extensions_support(device, config()->device_extensions) &&
          !SwapchainDetails(device, _surface).has_empty())
      {
        _physical_device = device;
        break;
      }
    }
  }

  throw_if(_physical_device == VK_NULL_HANDLE, "failed to find a suitable GPU");
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

  // device info 
  auto features = Features{};
  VkDeviceCreateInfo create_info
  {
    .sType                   = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
    .pNext                   = features.get(),
    .queueCreateInfoCount    = static_cast<uint32_t>(queue_infos.size()),
    .pQueueCreateInfos       = queue_infos.data(),
    .enabledExtensionCount   = static_cast<uint32_t>(config()->device_extensions.size()),
    .ppEnabledExtensionNames = config()->device_extensions.data(),
  };

  // create logical device
  throw_if(vkCreateDevice(_physical_device, &create_info, nullptr, &_device) != VK_SUCCESS,
           "failed to create device");

  _destructors.push([this] { vkDestroyDevice(_device, nullptr); });

  //
  // get queues
  //
  vkGetDeviceQueue(_device, queue_families.graphics_family.value(), 0, &_graphics_queue);
  vkGetDeviceQueue(_device, queue_families.present_family.value(), 0, &_present_queue);
}

void GraphicsEngine::init_memory_allocator()
{
  _mem_alloc.init(_physical_device, _device, _instance, config()->vulkan_version);
  _destructors.push([this] { _mem_alloc.destroy(); });
}

void GraphicsEngine::create_swapchain()
{
  _swapchain.init(_physical_device, _device, _surface, _window);
  _destructors.push([&] { _swapchain.destroy(); });
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
  _frames.init(_device, _command_pool, &_swapchain);
  _destructors.push([&] { _frames.destroy(); });
}

auto GraphicsEngine::get_swapchain_image_size() -> glm::vec2
{
  assert(_swapchain.size());
  auto size{ _swapchain.image(0).extent2D() };
  return { size.width, size.height };
}

void GraphicsEngine::resize_swapchain()
{
  _swapchain.resize();
}

void GraphicsEngine::create_sampler()
{
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
  throw_if(vkCreateSampler(_device, &sampler_info, nullptr, &_sampler) != VK_SUCCESS,
           "failed to create smaa sampler");
  _destructors.push([&] { vkDestroySampler(_device, _sampler, nullptr); });
}

void GraphicsEngine::init_text_engine()
{
  _text_engine.init(_mem_alloc);
  _destructors.push([&] { _text_engine.destroy(); });
}

void GraphicsEngine::load_fonts(std::vector<std::string_view> const& fonts)
{
  for (auto const& font : fonts)
    _text_engine.load_font(font);
}

void GraphicsEngine::init_gpu_resource()
{
  auto cmd = _command_pool.create_command().begin();

  // offscreen image creatation
  //auto extent = _swapchain.extent();
  //_offscreen_image = _mem_alloc.create_image(_swapchain.format(), extent.width, extent.height, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT);

  // preload glyphs
  _text_engine.preload_glyphs(cmd);

  cmd.end().submit_wait_free(_command_pool, _graphics_queue);

  _destructors.push([&]
  {
    //_offscreen_image.destroy();
  });
}

void GraphicsEngine::init_sdf_resources()
{
  _sdf_buffer.init(&_frames, &_mem_alloc);
  _sdf_graphics_pipeline.init({
    _device,
    {
      { ShaderType::fragment, 0, DescriptorType::sampler2D, _text_engine.get_glyph_atlases(), _sampler },
    },
    sizeof(PushConstant_SDF),
    _swapchain.format(),
    "shader/SDF_vert.spv",
    "shader/SDF_frag.spv"
  });

  _destructors.push([&]
  {
    _sdf_buffer.destroy();
    _sdf_graphics_pipeline.destroy();
  });
}

}}