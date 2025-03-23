#include "GraphicsEngine.hpp"
#include "init-util.hpp"

namespace tk
{

using namespace graphics_engine_init_util;

GraphicsEngine::GraphicsEngine(const Window& window)
  :_window(window)
{
  // only have single graphics engine
  static bool first = true;
  assert(first);
  if (first)  first = false;

  create_instance();
}

GraphicsEngine::~GraphicsEngine()
{
  vkDestroyInstance(_instance, nullptr);
}

void GraphicsEngine::create_instance()
{
  // debug messenger
#ifndef NDEBUG
  auto debug_messenger_info = get_debug_messenger_create_info();
#endif

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

  // create instance
  VkInstanceCreateInfo create_info =
  {
    .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
#ifndef NDEBUG
    .pNext = &debug_messenger_info,
#endif
#ifndef NDEBUG
    .enabledLayerCount   = 1,
    .ppEnabledLayerNames = &validation_layer,
#endif
    .enabledExtensionCount   = (uint32_t)extensions.size(),
    .ppEnabledExtensionNames = extensions.data(),
  };
  throw_if(vkCreateInstance(&create_info, nullptr, &_instance) != VK_SUCCESS,
           "failed to create vulkan instance!");
}

}
