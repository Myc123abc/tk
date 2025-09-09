#pragma once
  
#include <glm/glm.hpp>
#include <inttypes.h>

namespace tk { namespace graphics_engine {

  struct Vertex
  {
    glm::vec2 pos{};
    glm::vec2 uv{};
    uint32_t  offset{};
    uint32_t  glyph_atlases_index{};
  };

}}