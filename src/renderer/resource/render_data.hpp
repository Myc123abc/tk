#pragma once

#include "../shader/sdf/type.hpp"

#include <atomic>
#include <vector>

namespace tk { namespace renderer {

struct RenderData
{
  std::vector<Vertex>        vertices;
  std::vector<uint16_t>      indices;
  std::vector<ShapeProperty> shape_properties;

  auto is_using() const noexcept { return _is_using.load(std::memory_order_acquire); }
  auto use()    noexcept { return _is_using.store(true, std::memory_order_release);  }
  auto finish() noexcept { return _is_using.store(false, std::memory_order_release); }

  auto clear() noexcept
  {
    vertices.clear();
    indices.clear();
    shape_properties.clear();
  }

  auto empty() const noexcept { return vertices.empty(); }

private:
  std::atomic_bool _is_using;
};

}}
