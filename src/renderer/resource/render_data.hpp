#pragma once

#include "../shader/sdf/type.hpp"

#include <windows.h>

#include <vector>

namespace tk { namespace renderer {

struct RenderData
{
  std::vector<Vertex>        vertices;
  std::vector<uint16_t>      indices;
  std::vector<ShapeProperty> shape_properties;
  RECT                       scissor_rect{};
  glm::vec2                  resizing_window_pos;

  auto clear() noexcept
  {
    vertices.clear();
    indices.clear();
    shape_properties.clear();
    scissor_rect        = {};
    resizing_window_pos = {};
  }
};

}}
