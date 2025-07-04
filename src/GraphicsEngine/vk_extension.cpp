#include "tk/GraphicsEngine/GraphicsEngine.hpp"
#include "tk/ErrorHandling.hpp"
#include "tk/GraphicsEngine/vk_extension.hpp"
#include "tk/GraphicsEngine/config.hpp"

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
  if (config()->use_descriptor_buffer)
  {
    load_device_ext_func(_device, vkGetDescriptorSetLayoutSizeEXT);
    load_device_ext_func(_device, vkGetDescriptorSetLayoutBindingOffsetEXT);
    load_device_ext_func(_device, vkGetDescriptorEXT);
    load_device_ext_func(_device, vkCmdBindDescriptorBuffersEXT);
    load_device_ext_func(_device, vkCmdSetDescriptorBufferOffsetsEXT);
  }
  if (config()->use_shader_object)
  {
    load_device_ext_func(_device, vkCreateShadersEXT);
    load_device_ext_func(_device, vkDestroyShaderEXT);
    load_device_ext_func(_device, vkCmdBindShadersEXT);
    load_device_ext_func(_device, vkCmdSetCullModeEXT);
    load_device_ext_func(_device, vkCmdSetDepthWriteEnableEXT);
    load_device_ext_func(_device, vkCmdSetPolygonModeEXT);
    load_device_ext_func(_device, vkCmdSetRasterizationSamplesEXT);
    load_device_ext_func(_device, vkCmdSetSampleMaskEXT);
    load_device_ext_func(_device, vkCmdSetAlphaToCoverageEnableEXT);
    load_device_ext_func(_device, vkCmdSetVertexInputEXT);
    load_device_ext_func(_device, vkCmdSetColorBlendEnableEXT);
    load_device_ext_func(_device, vkCmdSetColorWriteMaskEXT);
    load_device_ext_func(_device, vkCmdSetColorBlendEquationEXT);
  }
}

}}