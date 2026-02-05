#pragma once

#include "../shader/sdf/type.hpp"
#include "../../util/object_pool.hpp"
#include "../config.hpp"

#include <vector>
#include <semaphore>

namespace tk { namespace renderer {

struct RenderData
{
  std::vector<Vertex>        vertices;
  std::vector<uint16_t>      indices;
  std::vector<ShapeProperty> shape_properties;
  std::binary_semaphore      sem{ 1 };
  glm::vec2                  resizing_window_pos;
  RECT                       scissor_rect{};

  auto clear() noexcept
  {
    vertices.clear();
    indices.clear();
    shape_properties.clear();
    sem.release();
    resizing_window_pos = {};
    scissor_rect        = {};
  }

  void wait() noexcept { sem.acquire(); }
};

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

private:
  using PoolType = ObjectPool<RenderData, RenderData_Pool_Init_Capacity>;
public:
  using RenderDataHandle = PoolType::Handle;

  auto& operator[](RenderDataHandle handle) noexcept { return *_pool.get(handle); }

  void free(RenderDataHandle& handle) noexcept { _pool.free(handle); }

private:
  PoolType _pool;
};

using RenderDataHandle = RenderDataPool::RenderDataHandle;

inline static auto& g_render_data_pool{ *RenderDataPool::instance() };

}}
