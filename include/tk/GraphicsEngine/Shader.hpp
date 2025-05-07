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
    glm::vec4 OriginalSize;
    glm::vec4 OutputSize;
    glm::vec4 smaa_rt_metrics;
    uint32_t  FrameCount;
    float     SMAA_EDT; // edge detection type
    float     SMAA_THRESHOLD;
    float     SMAA_MAX_SEARCH_STEPS;
    float     SMAA_MAX_SEARCH_STEPS_DIAG;
    float     SMAA_LOCAL_CONTRAST_ADAPTATION_FACTOR;
  };

}}