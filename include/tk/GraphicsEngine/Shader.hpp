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
  
  struct Vertex
  {
    alignas(8) glm::vec2 pos;
    alignas(8) glm::vec2 uv;
    alignas(8) uint32_t  col; // 0xRRGGBBAA
  };

}}