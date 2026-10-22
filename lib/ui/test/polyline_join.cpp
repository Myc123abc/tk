#include "polyline_tessellator.hpp"

#include <algorithm>
#include <array>
#include <cmath>

using namespace tk;

namespace {

auto nearly_equal(float lhs, float rhs) noexcept -> bool
{
  return std::abs(lhs - rhs) < .0001f;
}

auto clipped_bevel_follows_segment_edges(float turn_y) noexcept -> bool
{
  constexpr auto Thickness = 4.f;
  constexpr auto Aa_Size   = 1.f;
  constexpr auto Outer     = (Thickness + Aa_Size) * .5f;

  auto const points = std::array
  {
    float2{  0,      0 },
    float2{ 10,      0 },
    float2{  5, turn_y },
  };
  auto const next_delta  = points[2] - points[1];
  auto const next_normal = float2{ -next_delta.y, next_delta.x } / std::sqrt(length_sq(next_delta));
  auto const bevel_side  = turn_y > 0 ? -1.f : 1.f;
  auto const next_vertex = turn_y > 0 ? 4u : 5u;

  auto tessellator = ui::PolylineTessellator{};
  auto const mesh = tessellator.build(
    points,
    Thickness,
    Aa_Size,
    false,
    false,
    ui::PolylineTessellator::FringeMode::texture_coverage
  );
  if (mesh.vertices.size() != 8 || mesh.indices.size() != 21)
    return false;

  auto const join = points[1];
  auto const incoming_edge_offset = mesh.vertices[2].pos.y - join.y;
  auto const outgoing_edge_offset = dot(mesh.vertices[next_vertex].pos - join, next_normal);
  return nearly_equal(incoming_edge_offset, bevel_side * Outer) &&
         nearly_equal(outgoing_edge_offset, bevel_side * Outer);
}

auto miter_only_preserves_sharp_join(float turn_y) noexcept -> bool
{
  auto const points = std::array
  {
    float2{  0,      0 },
    float2{ 10,      0 },
    float2{  5, turn_y },
  };
  auto tessellator = ui::PolylineTessellator{};
  auto const mesh = tessellator.build(
    points,
    4.f,
    1.f,
    false,
    false,
    ui::PolylineTessellator::FringeMode::texture_coverage,
    true
  );
  auto pixel_snap_count = 0u;
  for (auto const& vertex : mesh.vertices)
    pixel_snap_count += vertex.pixel_snap;
  return mesh.vertices.size() == 6 && mesh.indices.size() == 12 &&
         pixel_snap_count == 1;
}

auto miter_only_preserves_closed_inside_join() noexcept -> bool
{
  auto const points = std::array
  {
    float2{  0,  0 },
    float2{ 10,  0 },
    float2{  5,  1 },
    float2{  0, 10 },
  };
  auto tessellator = ui::PolylineTessellator{};
  auto const mesh = tessellator.build(
    points,
    4.f,
    1.f,
    true,
    true,
    ui::PolylineTessellator::FringeMode::texture_coverage,
    true
  );
  return mesh.vertices.size() == points.size() * 2 &&
         mesh.indices.size() == points.size() * 6;
}

auto centered_miter_preserves_outline_position() noexcept -> bool
{
  auto const points = std::array
  {
    float2{  0,  0 },
    float2{ 10,  0 },
    float2{ 10, 10 },
    float2{  0, 10 },
  };
  auto tessellator = ui::PolylineTessellator{};
  auto const mesh = tessellator.build(
    points,
    1.f,
    1.f,
    true,
    false,
    ui::PolylineTessellator::FringeMode::texture_coverage,
    true
  );
  if (mesh.vertices.size() != points.size() * 2 ||
      mesh.indices.size() != points.size() * 6)
    return false;

  auto min = mesh.vertices.front().pos;
  auto max = min;
  for (auto const& vertex : mesh.vertices)
  {
    min.x = std::min(min.x, vertex.pos.x);
    min.y = std::min(min.y, vertex.pos.y);
    max.x = std::max(max.x, vertex.pos.x);
    max.y = std::max(max.y, vertex.pos.y);
  }
  return nearly_equal(min.x, -1.f) &&
         nearly_equal(min.y, -1.f) &&
         nearly_equal(max.x, 11.f) &&
         nearly_equal(max.y, 11.f);
}

}

int main()
{
  return clipped_bevel_follows_segment_edges(1.f) &&
         clipped_bevel_follows_segment_edges(-1.f) &&
         miter_only_preserves_sharp_join(1.f) &&
         miter_only_preserves_sharp_join(-1.f) &&
         miter_only_preserves_closed_inside_join() &&
         centered_miter_preserves_outline_position()
    ? 0
    : 1;
}