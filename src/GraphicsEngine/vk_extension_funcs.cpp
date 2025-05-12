#include "tk/GraphicsEngine/GraphicsEngine.hpp"
#include "tk/ErrorHandling.hpp"

namespace tk { namespace graphics_engine {

void GraphicsEngine::load_instance_extension_funcs()
{
#ifndef NDEBUG
  vkCreateDebugUtilsMessengerEXT = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(_instance, "vkCreateDebugUtilsMessengerEXT");
  throw_if(!vkCreateDebugUtilsMessengerEXT, "failed to load vkCreateDebugUtilsMessengerEXT");
  vkDestroyDebugUtilsMessengerEXT = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(_instance, "vkDestroyDebugUtilsMessengerEXT");
  throw_if(!vkDestroyDebugUtilsMessengerEXT, "failed to load vkDestroyDebugUtilsMessengerEXT");
#endif
}

void GraphicsEngine::load_device_extension_funcs()
{
  vkGetDescriptorSetLayoutSizeEXT = (PFN_vkGetDescriptorSetLayoutSizeEXT)vkGetDeviceProcAddr(_device, "vkGetDescriptorSetLayoutSizeEXT");
  throw_if(!vkGetDescriptorSetLayoutSizeEXT, "failed to load vkGetDescriptorSetLayoutSizeEXT");
}

}}