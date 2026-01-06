#pragma once

#include "../shader/sdf/type.hpp"
#include "../../util/object_pool.hpp"
#include "../config.hpp"

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

using RenderDataPoolType = ObjectPool<RenderData, RenderData_Pool_Init_Capacity>;
using RenderDataHandle   = RenderDataPoolType::Handle;
class RenderDataPool
{
private:
  RenderDataPool()                                 = default;
  ~RenderDataPool()                                = default;
public:
  RenderDataPool(RenderDataPool const&)            = delete;
  RenderDataPool(RenderDataPool&&)                 = delete;
  RenderDataPool& operator=(RenderDataPool const&) = delete;
  RenderDataPool& operator=(RenderDataPool&&)      = delete;

  static auto const instance() noexcept
  {
    static RenderDataPool instance;
    return &instance;
  }

  auto alloc() noexcept { return _pool.alloc(); }

  auto& operator[](RenderDataHandle handle) noexcept { return *_pool.get(handle); }

  void free(RenderDataHandle& handle) noexcept { _pool.free(handle); }

private:
  RenderDataPoolType _pool;
};

inline static auto& g_render_data_pool{ *RenderDataPool::instance() };

}}
