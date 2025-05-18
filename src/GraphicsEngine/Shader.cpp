#include "tk/GraphicsEngine/Shader.hpp"
#include "tk/GraphicsEngine/vk_extension.hpp"

#include <cassert>

namespace tk { namespace graphics_engine {

void Shader::destroy()
{
  assert(_device && _shader);
  vkDestroyShaderEXT(_device, _shader, nullptr);
  _device = {};
  _shader = {};
}

void Shader::set(VkDevice device, VkShaderEXT shader) noexcept
{
  assert(device && shader && !_device && !_shader);
  _device = device;
  _shader = shader;
}


}}