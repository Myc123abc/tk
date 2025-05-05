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

}}