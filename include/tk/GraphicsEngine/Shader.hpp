//
// Shader something
//

#pragma once

#include <glm/glm.hpp>
#include <vulkan/vulkan.h>

namespace tk { namespace graphics_engine {

  class Shader final
  {
  public:
    Shader()  = default;
    ~Shader() = default;

    operator VkShaderEXT() const noexcept { return _shader; }

    void destroy();

  private:
    friend class Device;

    void set(VkDevice device, VkShaderEXT shader) noexcept;

  private:
    VkDevice    _device{};
    VkShaderEXT _shader{};
  };

}}