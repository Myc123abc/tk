#include "frame_data.hpp"
#include "util.hpp"

namespace tk::ui {

void FrameData::path_begin(float2 p0) noexcept
{
  auto& cmd = _draw_cmds.emplace_back(DrawCmd::Type::path_begin);
  cmd.data.path_begin = { p0 };
}

void FrameData::_path_begin(float2 p0) noexcept
{
  assert(_points.empty());
  _points.emplace_back(p0);
}

void FrameData::add_path_line_to(float2 p) noexcept
{
  auto& cmd = _draw_cmds.emplace_back(DrawCmd::Type::add_path_line_to);
  cmd.data.add_path_line_to = { p };
}

void FrameData::add_path_arc_to(float2 center, float2 p1, bool ccw) noexcept
{
  auto& cmd = _draw_cmds.emplace_back(DrawCmd::Type::add_path_arc_to);
  cmd.data.add_path_arc_to = { center, p1, ccw };
}

void FrameData::_add_path_arc_to(float2 center, float2 p1, bool ccw, float tessellation_scale) noexcept
{
  assert(!_points.empty());
  auto p0 = _points.back(); _points.pop_back();
  auto radius = length(p0 - center);
  if (radius * tessellation_scale < .5f)
  {
    _points.emplace_back(p1);
    return;
  }

  auto a0 = angle_of(center, p0);
  auto a1 = angle_of(center, p1);
  if (ccw) std::swap(a0, a1);
  path_arc_to(center, radius, a0, a1, tessellation_scale);
}

void FrameData::add_path_quad_bezier_to(float2 p1, float2 p2) noexcept
{
  auto& cmd = _draw_cmds.emplace_back(DrawCmd::Type::add_path_quad_bezier_to);
  cmd.data.add_path_quad_bezier_to = { p1, p2 };
}

void FrameData::_add_path_quad_bezier_to(float2 p1, float2 p2, float tessellation_scale) noexcept
{
  assert(!_points.empty());
  auto const max_error = curve_tessellation_max_error / tessellation_scale;
  path_bezier_quad_curve_to_casteljau(_points.back(), p1, p2, max_error * max_error, 0);
}

void FrameData::add_path_cubic_bezier_to(float2 p1, float2 p2, float2 p3) noexcept
{
  auto& cmd = _draw_cmds.emplace_back(DrawCmd::Type::add_path_cubic_bezier_to);
  cmd.data.add_path_cubic_bezier_to = { p1, p2, p3 };
}

void FrameData::_add_path_cubic_bezier_to(float2 p1, float2 p2, float2 p3, float tessellation_scale) noexcept
{
  assert(!_points.empty());
  auto const max_error = curve_tessellation_max_error / tessellation_scale;
  path_bezier_cubic_curve_to_casteljau(_points.back(), p1, p2, p3, max_error * max_error, 0);
}

void FrameData::path_end(bool close, Color color, float thickness) noexcept
{
  auto& cmd = _draw_cmds.emplace_back(DrawCmd::Type::path_end);
  cmd.data.path_end = { close, color, thickness };
}

void FrameData::_path_end(bool close, Color color, float thickness, float geometry_scale, bool transform_geometry) noexcept
{
  if (_points.size() < 2)
  {
    _points.clear();
    return;
  }

  if (_using_discard_shapes) color.a = 1.f;
  else if (color.a == 0)
  {
    _points.clear();
    return;
  }

  if (thickness > 0)
    add_poly_line(color, thickness, close, false, true, geometry_scale, transform_geometry);
  else
  {
    if (is_convex(_points))
      add_convex_poly_filled(color);
    else
      add_concave_poly_filled(color);
  }
}

void FrameData::path_bezier_quad_curve_to_casteljau(float2 p0, float2 p1, float2 p2, float max_error_sq, int level) noexcept
{
  assert(std::isfinite(max_error_sq) && max_error_sq > 0.f);

  auto dp  = p2 - p0;
  auto det = cross(p1 - p2, dp);
  if (det * det * 4.f < max_error_sq * length_sq(dp))
    _points.emplace_back(p2);
  else if (level < 10)
  {
    auto p01  = ( p0 +  p1) * .5f;
    auto p12  = ( p1 +  p2) * .5f;
    auto p012 = (p01 + p12) * .5f;
    path_bezier_quad_curve_to_casteljau(p0, p01, p012, max_error_sq, level + 1);
    path_bezier_quad_curve_to_casteljau(p012, p12, p2, max_error_sq, level + 1);
  }
}

void FrameData::path_bezier_cubic_curve_to_casteljau(float2 p0, float2 p1, float2 p2, float2 p3, float max_error_sq, int level) noexcept
{
  assert(std::isfinite(max_error_sq) && max_error_sq > 0.f);

  auto dp = p3 - p0;
  auto d2 = std::abs(cross(p1 - p3, dp));
  auto d3 = std::abs(cross(p2 - p3, dp));
  if ((d2 + d3) * (d2 + d3) < max_error_sq * length_sq(dp))
    _points.emplace_back(p3);
  else if (level < 10)
  {
    auto p01   = (  p0 +   p1) * .5f;
    auto p12   = (  p1 +   p2) * .5f;
    auto p23   = (  p2 +   p3) * .5f;
    auto p012  = ( p01 +  p12) * .5f;
    auto p123  = ( p12 +  p23) * .5f;
    auto p0123 = (p012 + p123) * .5f;
    path_bezier_cubic_curve_to_casteljau(p0, p01, p012, p0123, max_error_sq, level + 1);
    path_bezier_cubic_curve_to_casteljau(p0123, p123, p23, p3, max_error_sq, level + 1);
  }
}


}
