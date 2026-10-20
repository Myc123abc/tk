#pragma once

#include "tk/base.hpp"

#include <span>
#include <vector>

namespace tk::ui {

inline constexpr auto Polyline_Miter_Angle_Limit = -0.9999619f;
inline constexpr auto Polyline_Miter_Limit       = 4.f;

class PolylineTessellator
{
public:
  enum class FringeMode
  {
    geometry,
    texture_coverage,
  };

  struct Vertex
  {
    float2 pos;
    bool   opaque;
    float  coverage{};
  };

  struct Mesh
  {
    // Views remain valid only until the next build() or clear() call.
    std::span<Vertex const> vertices;
    std::span<uint const>   indices;
    uint                    core_vertex_count{};
    uint                    core_index_count{};
    uint                    boundary_edge_count{};
  };

  auto build(
    std::span<float2 const> points,
    float                   thickness,
    float                   aa_size,
    bool                    is_closed,
    bool                    stroke_inside,
    FringeMode              fringe_mode = FringeMode::geometry
  ) noexcept -> Mesh;

  void clear() noexcept;

private:
  enum class Side : int
  {
    negative = -1,
    center,
    positive,
  };

  struct CorePoint
  {
    float2 pos;
    float2 offset;
    Side   side;
  };

  struct StrokeOffsets
  {
    float negative_inner;
    float negative_outer;
    float positive_inner;
    float positive_outer;

    auto inner(Side side) const noexcept -> float;
    auto outer(Side side) const noexcept -> float;
    auto max_outer() const noexcept -> float;
  };

  static auto calculate_stroke_offsets(
    std::span<float2 const> points,
    float                   thickness,
    float                   aa_size,
    bool                    stroke_inside
  ) noexcept -> StrokeOffsets;

  void calculate_normals(std::span<float2 const> points, bool is_closed) noexcept;
  void build_core(std::span<float2 const> points, bool is_closed, float max_outer) noexcept;
  auto collect_boundary_edges() noexcept -> uint;
  void build_fringe_mesh(StrokeOffsets const& offsets, uint boundary_edge_count) noexcept;
  void build_texture_mesh(StrokeOffsets const& offsets) noexcept;

  auto append_core_point(float2 pos, float2 offset, Side side) -> uint;
  void append_core_triangle(uint p0, uint p1, uint p2);

  std::vector<float2>    _normals;
  std::vector<float>     _sq_lengths;
  std::vector<CorePoint> _core_points;
  std::vector<uint>      _core_indices;
  std::vector<uint64>    _edge_keys;
  std::vector<Vertex>    _vertices;
  std::vector<uint>      _indices;
};

}
