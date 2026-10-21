#include "polyline_tessellator.hpp"

#include <algorithm>
#include <cassert>
#include <cmath>

namespace tk::ui {

auto PolylineTessellator::StrokeOffsets::inner(Side side) const noexcept -> float
{
  if (side == Side::negative) return negative_inner;
  if (side == Side::positive) return positive_inner;
  return 0;
}

auto PolylineTessellator::StrokeOffsets::outer(Side side) const noexcept -> float
{
  if (side == Side::negative) return negative_outer;
  if (side == Side::positive) return positive_outer;
  return 0;
}

auto PolylineTessellator::StrokeOffsets::max_outer() const noexcept -> float
{
  return std::max(negative_outer, positive_outer);
}

auto PolylineTessellator::calculate_stroke_offsets(
  std::span<float2 const> points,
  float                   thickness,
  float                   aa_size,
  bool                    stroke_inside
) noexcept -> StrokeOffsets
{
  auto offsets = StrokeOffsets
  {
    .negative_inner = (thickness - aa_size) * .5f,
    .negative_outer = (thickness + aa_size) * .5f,
    .positive_inner = (thickness - aa_size) * .5f,
    .positive_outer = (thickness + aa_size) * .5f,
  };
  if (!stroke_inside)
    return offsets;

  offsets.negative_inner = -.5f * aa_size;
  offsets.negative_outer =  .5f * aa_size;
  offsets.positive_inner = thickness - .5f * aa_size;
  offsets.positive_outer = thickness + .5f * aa_size;

  auto winding = 0.f;
  for (auto index = 0u; index < points.size(); ++index)
    winding += cross(points[index], points[(index + 1) % points.size()]);
  if (winding < 0)
  {
    std::swap(offsets.negative_inner, offsets.positive_inner);
    std::swap(offsets.negative_outer, offsets.positive_outer);
  }
  return offsets;
}

void PolylineTessellator::calculate_normals(std::span<float2 const> points, bool is_closed) noexcept
{
  auto const point_count = static_cast<uint>(points.size());
  _normals.resize(point_count);
  _sq_lengths.resize(point_count);

  for (auto index = 0u; index + 1 < point_count; ++index)
  {
    auto const delta = points[index + 1] - points[index];
    auto const length_squared = length_sq(delta);
    auto const direction = length_squared > 0 ? delta / std::sqrt(length_squared) : float2{};
    _normals[index] = { -direction.y, direction.x };
    _sq_lengths[index] = length_squared;
  }

  if (is_closed)
  {
    auto const delta = points.front() - points.back();
    auto const length_squared = length_sq(delta);
    auto const direction = length_squared > 0 ? delta / std::sqrt(length_squared) : float2{};
    _normals.back() = { -direction.y, direction.x };
    _sq_lengths.back() = length_squared;
  }
  else
  {
    _normals.back() = _normals[point_count - 2];
    _sq_lengths.back() = 0;
  }
}

auto PolylineTessellator::append_core_point(float2 pos, float2 offset, Side side, bool pixel_snap) -> uint
{
  auto const index = static_cast<uint>(_core_points.size());
  _core_points.emplace_back(pos, offset, side, pixel_snap);
  return index;
}

void PolylineTessellator::append_core_triangle(uint p0, uint p1, uint p2)
{
  _core_indices.emplace_back(p0);
  _core_indices.emplace_back(p1);
  _core_indices.emplace_back(p2);
}

void PolylineTessellator::build_core(
  std::span<float2 const> points,
  bool                    is_closed,
  float                   max_outer,
  bool                    miter_only
) noexcept
{
  auto const point_count = static_cast<uint>(points.size());
  _core_points.clear();
  _core_indices.clear();
  _core_points.reserve(point_count * 4 + 2);
  _core_indices.reserve(point_count * 15 + 6);

  auto point_index = 0u;
  auto point_end   = point_count;
  auto point       = float2{};
  auto normal      = float2{};
  auto length_squared = 0.f;
  auto base_index  = 0u;

  if (is_closed)
  {
    point = points.back();
    normal = _normals.back();
    length_squared = _sq_lengths.back();
    base_index = append_core_point({}, {}, Side::negative);
    append_core_point({}, {}, Side::positive);
  }
  else
  {
    point_index = 1;
    --point_end;
    point = points.front();
    normal = _normals.front();
    length_squared = _sq_lengths.front();
    base_index = append_core_point(point, normal * -1.f, Side::negative);
    append_core_point(point, normal, Side::positive);
  }

  while (point_index < point_end)
  {
    auto const previous_normal = normal;
    auto const previous_length_squared = length_squared;
    point = points[point_index];
    normal = _normals[point_index];
    length_squared = _sq_lengths[point_index];

    auto const cos_theta = dot(previous_normal, normal);
    auto const sin_theta = -cross(previous_normal, normal);
    auto const clamped_cos = std::max(Polyline_Miter_Angle_Limit, cos_theta);
    auto const miter_scale = std::min(1000.f, 1.f / (1.f + clamped_cos));
    auto miter = (previous_normal + normal) * miter_scale;
    auto const miter_distance_squared = length_sq(miter);

    if (!miter_only && miter_distance_squared > Polyline_Miter_Limit * Polyline_Miter_Limit)
    {
      auto const reference_half_squared = max_outer * max_outer;
      auto const limit_squared = std::max({ previous_length_squared, length_squared, reference_half_squared });
      auto const reference_miter_squared = miter_distance_squared * reference_half_squared;
      if (reference_miter_squared > limit_squared)
        miter *= std::sqrt(limit_squared / reference_miter_squared);

      auto const bevel_normal = normalize(previous_normal + normal);
      auto const denominator = -cross(previous_normal, bevel_normal);
      auto const side_offset = denominator != 0
        ? (dot(previous_normal, bevel_normal) - 1.f) / denominator
        : 0.f;
      auto const side_delta = float2{ bevel_normal.y, -bevel_normal.x } * side_offset;

      if (sin_theta < 0)
      {
        auto const outer_previous = append_core_point(point, (bevel_normal + side_delta) * -1.f, Side::negative);
        auto const center         = append_core_point(point, {}, Side::center);
        auto const next_base      = append_core_point(point, (bevel_normal - side_delta) * -1.f, Side::negative);
        auto const inner          = append_core_point(point, miter, Side::positive);
        append_core_triangle(base_index, outer_previous, inner);
        append_core_triangle(base_index, inner, base_index + 1);
        append_core_triangle(center, inner, outer_previous);
        append_core_triangle(center, outer_previous, next_base);
        append_core_triangle(center, next_base, inner);
        base_index = next_base;
      }
      else
      {
        auto const outer_previous = append_core_point(point, bevel_normal + side_delta, Side::positive);
        auto const center         = append_core_point(point, {}, Side::center);
        auto const next_base      = append_core_point(point, miter * -1.f, Side::negative);
        auto const outer_next     = append_core_point(point, bevel_normal - side_delta, Side::positive);
        append_core_triangle(base_index, next_base, outer_previous);
        append_core_triangle(base_index, outer_previous, base_index + 1);
        append_core_triangle(center, outer_previous, next_base);
        append_core_triangle(center, next_base, outer_next);
        append_core_triangle(center, outer_next, outer_previous);
        base_index = next_base;
      }
    }
    else
    {
      auto const sharp_miter = miter_only && miter_distance_squared > Polyline_Miter_Limit * Polyline_Miter_Limit;
      auto const next_base   = append_core_point(point, miter * -1.f, Side::negative, sharp_miter && sin_theta < 0);
      append_core_point(point, miter, Side::positive, sharp_miter && sin_theta >= 0);
      append_core_triangle(base_index, next_base, next_base + 1);
      append_core_triangle(base_index, next_base + 1, base_index + 1);
      base_index = next_base;
    }

    ++point_index;
  }

  if (is_closed)
  {
    assert(base_index + 1 == _core_points.size() - 1);
    _core_points[0] = _core_points[base_index];
    _core_points[1] = _core_points[base_index + 1];
    for (auto& index : _core_indices)
    {
      if (index == base_index) index = 0;
      else if (index == base_index + 1) index = 1;
    }
    _core_points.resize(_core_points.size() - 2);
  }
  else
  {
    point = points.back();
    normal = _normals.back();
    auto const next_base = append_core_point(point, normal * -1.f, Side::negative);
    append_core_point(point, normal, Side::positive);
    append_core_triangle(base_index, next_base, next_base + 1);
    append_core_triangle(base_index, next_base + 1, base_index + 1);
  }
}

auto PolylineTessellator::collect_boundary_edges() noexcept -> uint
{
  _edge_keys.clear();
  _edge_keys.reserve(_core_indices.size());

  auto append_edge = [&](uint p0, uint p1)
  {
    auto const min = std::min(p0, p1);
    auto const max = std::max(p0, p1);
    _edge_keys.emplace_back(static_cast<uint64>(min) << 32 | max);
  };
  for (auto index = 0u; index < _core_indices.size(); index += 3)
  {
    append_edge(_core_indices[index],     _core_indices[index + 1]);
    append_edge(_core_indices[index + 1], _core_indices[index + 2]);
    append_edge(_core_indices[index + 2], _core_indices[index]);
  }
  std::ranges::sort(_edge_keys);

  auto boundary_edge_count = 0u;
  for (auto begin = 0u; begin < _edge_keys.size();)
  {
    auto end = begin + 1;
    while (end < _edge_keys.size() && _edge_keys[end] == _edge_keys[begin]) ++end;
    boundary_edge_count += end - begin == 1;
    begin = end;
  }
  return boundary_edge_count;
}

void PolylineTessellator::build_fringe_mesh(
  StrokeOffsets const& offsets,
  uint                 boundary_edge_count
) noexcept
{
  _vertices.resize(_core_points.size() * 2);
  for (auto index = 0u; index < _core_points.size(); ++index)
  {
    auto const& point = _core_points[index];
    _vertices[index * 2]     = { point.pos + point.offset * offsets.inner(point.side), true,  {}, point.pixel_snap };
    _vertices[index * 2 + 1] = { point.pos + point.offset * offsets.outer(point.side), false, {}, point.pixel_snap };
  }

  _indices.resize(_core_indices.size() + boundary_edge_count * 6);
  auto* output = _indices.data();
  for (auto index : _core_indices)
    *output++ = index * 2;

  for (auto begin = 0u; begin < _edge_keys.size();)
  {
    auto end = begin + 1;
    while (end < _edge_keys.size() && _edge_keys[end] == _edge_keys[begin]) ++end;
    if (end - begin == 1)
    {
      auto const p0 = static_cast<uint>(_edge_keys[begin] >> 32) * 2;
      auto const p1 = static_cast<uint>(_edge_keys[begin]) * 2;
      uint const fringe_indices[] = { p0, p1, p1 + 1, p0, p1 + 1, p0 + 1 };
      for (auto index : fringe_indices)
        *output++ = index;
    }
    begin = end;
  }
  assert(output == _indices.data() + _indices.size());
}

void PolylineTessellator::build_texture_mesh(StrokeOffsets const& offsets) noexcept
{
  _vertices.resize(_core_points.size());
  auto const center_coverage = offsets.negative_outer / (offsets.negative_outer + offsets.positive_outer);
  for (auto index = 0u; index < _core_points.size(); ++index)
  {
    auto const& point = _core_points[index];
    auto const coverage = point.side == Side::negative ? 0.f
      : point.side == Side::positive ? 1.f
      : center_coverage;
    _vertices[index] = { point.pos + point.offset * offsets.outer(point.side), true, coverage, point.pixel_snap };
  }
  _indices = _core_indices;
}

auto PolylineTessellator::build(
  std::span<float2 const> points,
  float                   thickness,
  float                   aa_size,
  bool                    is_closed,
  bool                    stroke_inside,
  FringeMode              fringe_mode,
  bool                    miter_only
) noexcept -> Mesh
{
  if (points.size() < 2 || aa_size <= 0 || thickness < aa_size || (stroke_inside && !is_closed))
  {
    clear();
    return {};
  }

  assert(points.size() >= 2);
  assert(thickness >= aa_size);
  assert(aa_size > 0);
  assert(!stroke_inside || is_closed);

  auto const offsets = calculate_stroke_offsets(points, thickness, aa_size, stroke_inside);
  calculate_normals(points, is_closed);
  build_core(points, is_closed, offsets.max_outer(), miter_only);
  auto boundary_edge_count = 0u;
  if (fringe_mode == FringeMode::geometry)
  {
    boundary_edge_count = collect_boundary_edges();
    build_fringe_mesh(offsets, boundary_edge_count);
  }
  else
    build_texture_mesh(offsets);

  return
  {
    .vertices            = _vertices,
    .indices             = _indices,
    .core_vertex_count   = static_cast<uint>(_core_points.size()),
    .core_index_count    = static_cast<uint>(_core_indices.size()),
    .boundary_edge_count = boundary_edge_count,
  };
}

void PolylineTessellator::clear() noexcept
{
  _normals.clear();
  _sq_lengths.clear();
  _core_points.clear();
  _core_indices.clear();
  _edge_keys.clear();
  _vertices.clear();
  _indices.clear();
}

}
