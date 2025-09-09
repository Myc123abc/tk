#include "GraphicsEngine.hpp"
#include "../ErrorHandling.hpp"
#include "vk_extension.hpp"

#include <string_view>

namespace tk { namespace graphics_engine {

template <typename T>
inline void load_instance_ext_func(VkInstance instance, T& func, std::string_view func_name)
{
  func = reinterpret_cast<T>(vkGetInstanceProcAddr(instance, func_name.data()));
  throw_if(!func, "failed to load {}", func_name);
}

template <typename T>
inline void load_device_ext_func(VkDevice device, T& func, std::string_view func_name)
{
  func = reinterpret_cast<T>(vkGetDeviceProcAddr(device, func_name.data()));
  throw_if(!func, "failed to load {}", func_name);
}

#define VAR_NAME(x) #x
#define load_instance_ext_func(instance, func) load_instance_ext_func(instance, func, VAR_NAME(func))
#define load_device_ext_func(device, func)     load_device_ext_func(device, func, VAR_NAME(func))

void GraphicsEngine::load_instance_extension_funcs()
{
#ifndef NDEBUG
  load_instance_ext_func(_instance, vkCreateDebugUtilsMessengerEXT);
  load_instance_ext_func(_instance, vkDestroyDebugUtilsMessengerEXT);
#endif
}

void GraphicsEngine::load_device_extension_funcs()
{

}

}}