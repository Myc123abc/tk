//
// Shader something
//
// pushconstant, vertex
//

#pragma once

#include <glm/glm.hpp>
#include <vulkan/vulkan.h>

namespace tk { namespace graphics_engine {

  struct PushConstant 
  {
    VkDeviceAddress vertices = {};
    glm::vec2       window_extent;
    glm::vec2       display_pos;
  };
  
  struct alignas(8) Vertex
  {
    glm::vec2 pos;
    glm::vec2 uv;
    uint32_t  col; // 0xRRGGBBAA
  };

  //
  // smaa
  // 
  struct PushConstant_SMAA
  {
    // TODO: maybe i can only transform vec2 and calculate tr_metrics in shader
    glm::vec4 smaa_rt_metrics;
  };

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