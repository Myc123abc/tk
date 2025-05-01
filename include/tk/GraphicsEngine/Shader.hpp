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
    glm::vec2       scale;
    glm::vec2       translate;
  };
  
  struct Vertex
  {
    glm::vec2 pos;
    glm::vec2 uv;
    uint32_t  col; // 0xAARRGGBB
  };

}}