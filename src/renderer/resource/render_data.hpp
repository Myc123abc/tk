#pragma once

#include "../shader/sdf/type.hpp"

#include <vector>

namespace tk { namespace renderer {

struct RenderData
{
  std::vector<Vertex>        vertices;
  std::vector<uint16_t>      indices;
  std::vector<ShapeProperty> shape_properties;

  auto clear() noexcept
  {
    vertices.clear();
    indices.clear();
    shape_properties.clear();
  }

  auto empty() const noexcept { return vertices.empty(); }
};

}}
