#include "frame_data.hpp"
#include "ui_context.hpp"
#include "triangulator.hpp"

#include <clipper2/clipper.h>

#include <numbers>

using namespace tk;

namespace {

auto fast_rsqrt(float x) noexcept
{
  return _mm_cvtss_f32(_mm_rsqrt_ss(_mm_set_ss(x)));
}

auto fast_normalize(float2 p) noexcept
{
  auto d2 = dot(p, p);
  if (d2 > .0f)
    return p * fast_rsqrt(d2);
  return float2{};
}

auto fix_normal(float2 p) noexcept
{
  auto d2 = dot(p, p);
  if (d2 > 0.000001f)
  {
    auto inv_len2 = 1.f / d2;
    auto const max = 100.f;
    if (inv_len2 > max)
      inv_len2 = max;
    return p * inv_len2;
  }
  return float2{};
}

auto round_up_to_even(int x) noexcept
{
  return ((x + 1) / 2) * 2;
}

auto calc_circle_radius(float cnt, float max_error) noexcept
{
  return max_error / (1 - std::cos(std::numbers::pi_v<float> / std::max(cnt, std::numbers::pi_v<float>)));
}

auto is_convex(std::span<float2> p) noexcept
{
  auto const n = p.size();
  if (n < 3)
    return false;

  auto sign = 0.0f;
  for (auto i = 0u; i < n; ++i)
  {
    auto const a = p[i];
    auto const b = p[(i + 1) % n];
    auto const c = p[(i + 2) % n];
    auto const v = cross(b - a, c - b);

    if (std::abs(v) < 1e-6f)
      continue;
    if (sign == .0f)
      sign = v;
    else if ((v > 0.0f) != (sign > 0.0f))
      return false;
  }

  return true;
}

auto angle_of(float2 center, float2 p) noexcept
{
  return std::atan2(p.y - center.y, p.x - center.x);
}

}

namespace tk::ui {

void FrameData::init() noexcept
{
  _circle_segment_counts[0] = arc_sample_max;
  for (auto i = 1; i < _circle_segment_counts.size(); ++i)
    _circle_segment_counts[i] = calc_circle_segment_count(static_cast<float>(i));

  for (auto i = 0; i < _arc_vertices.size(); ++i)
  {
    auto const a = i * 2 * std::numbers::pi / _arc_vertices.size();
    _arc_vertices[i] = { std::cos(a), std::sin(a) };
  }

  arc_radius_cutoff = calc_circle_radius(arc_sample_max, tessellation_max_error);
}

auto FrameData::calc_circle_segment_count(float radius) noexcept -> float
{
  auto constexpr segment_min = 4;
  auto constexpr segment_max = 512;
  return std::clamp(
    round_up_to_even(std::ceil(
      std::numbers::pi /
      std::acos(1 - std::min(tessellation_max_error, radius) / radius)
    )),
    segment_min, segment_max);
}

void FrameData::add_rect(float2 left_top, float2 right_bottom, Color color, float thickness) noexcept
{
  auto& cmd = _draw_cmds.emplace_back(DrawCmd::Type::add_rect);
  cmd.data.add_rect = { left_top, right_bottom, color, thickness };
}

void FrameData::_add_rect(float2 left_top, float2 right_bottom, Color color, float thickness) noexcept
{
  if (_using_discard_shapes) color.a = 1.f;
  else if (color.a == 0) return;

  if (thickness > 0)
  {
    left_top     += .5f;
    right_bottom -= .5f;
    assert(_points.empty());
    _points.emplace_back(left_top);
    _points.emplace_back(right_bottom.x, left_top.y);
    _points.emplace_back(right_bottom);
    _points.emplace_back(left_top.x, right_bottom.y);
    add_poly_line(color, thickness, true);
    return;
  }

  add_rect(left_top, right_bottom, color);
}

void FrameData::add_rect(float2 left_top, float2 right_bottom, Color color) noexcept
{
  auto [vertices, indices] = expand_beg(4, 6);

  vertices[0] = { left_top, {}, color };
  vertices[1] = { { right_bottom.x, left_top.y }, {}, color };
  vertices[2] = { right_bottom, {}, color };
  vertices[3] = { { left_top.x, right_bottom.y }, {}, color };
  indices[0]  = static_cast<uint16>(_vertex_beg + 0);
  indices[1]  = static_cast<uint16>(_vertex_beg + 1);
  indices[2]  = static_cast<uint16>(_vertex_beg + 2);
  indices[3]  = static_cast<uint16>(_vertex_beg + 0);
  indices[4]  = static_cast<uint16>(_vertex_beg + 2);
  indices[5]  = static_cast<uint16>(_vertex_beg + 3);

  expand_end();
}

void FrameData::add_triangle(float2 p0, float2 p1, float2 p2, Color color, float thickness) noexcept
{
  auto& cmd = _draw_cmds.emplace_back(DrawCmd::Type::add_triangle);
  cmd.data.add_triangle = { p0, p1, p2, color, thickness };
}

void FrameData::_add_triangle(float2 p0, float2 p1, float2 p2, Color color, float thickness) noexcept
{
  if (_using_discard_shapes) color.a = 1.f;
  else if (color.a == 0) return;

  assert(_points.empty());
  _points.emplace_back(p0);
  _points.emplace_back(p1);
  _points.emplace_back(p2);

  if (thickness > 0)
    add_poly_line(color, thickness, true);
  else
    add_convex_poly_filled(color);
}

void FrameData::add_circle(float2 center, float radius, Color color, float thickness) noexcept
{
  auto& cmd = _draw_cmds.emplace_back(DrawCmd::Type::add_circle);
  cmd.data.add_circle = { center, radius, color, thickness };
}

void FrameData::_add_circle(float2 center, float radius, Color color, float thickness) noexcept
{
  if (_using_discard_shapes) color.a = 1.f;
  else if (color.a == 0) return;

  assert(_points.empty());
  path_arc_to(center, radius - .5f, 0, std::numbers::pi_v<float> * 2.f);
  if (!_points.empty())
    _points.pop_back();

  if (thickness > 0)
    add_poly_line(color, thickness, true);
  else
    add_convex_poly_filled(color);
}

void FrameData::add_line(float2 p0, float2 p1, Color color, float thickness) noexcept
{
  auto& cmd = _draw_cmds.emplace_back(DrawCmd::Type::add_line);
  cmd.data.add_line = { p0, p1, color, thickness };
}

void FrameData::_add_line(float2 p0, float2 p1, Color color, float thickness) noexcept
{
  if (_using_discard_shapes) color.a = 1.f;
  else if (color.a == 0) return;

  assert(_points.empty());
  _points.emplace_back(p0);
  _points.emplace_back(p1);
  add_poly_line(color, thickness, false);
}

void FrameData::add_arc(float2 center, float2 p0, float2 p1, Color color, float thickness) noexcept
{
  auto& cmd = _draw_cmds.emplace_back(DrawCmd::Type::add_arc);
  cmd.data.add_arc = { center, p0, p1, color, thickness };
}

void FrameData::_add_arc(float2 center, float2 p0, float2 p1, Color color, float thickness) noexcept
{
  if (_using_discard_shapes) color.a = 1.f;
  else if (color.a == 0) return;

  auto radius = length(p0 - center);
  if (radius < .5f)
    return;

  assert(_points.empty());
  path_arc_to(center, radius, angle_of(center, p0), angle_of(center, p1));
  add_poly_line(color, thickness, false);
}

void FrameData::add_quad_bezier(float2 p0, float2 p1, float2 p2, Color color, float thickness) noexcept
{
  auto& cmd = _draw_cmds.emplace_back(DrawCmd::Type::add_quad_bezier);
  cmd.data.add_quad_bezier = { p0, p1, p2, color, thickness };
}

void FrameData::_add_quad_bezier(float2 p0, float2 p1, float2 p2, Color color, float thickness) noexcept
{
  if (_using_discard_shapes) color.a = 1.f;
  else if (color.a == 0) return;

  assert(_points.empty());
  _points.emplace_back(p0);
  path_bezier_quad_curve_to_casteljau(p0, p1, p2, curve_tessellation_tol, 0);
  add_poly_line(color, thickness, false);
}

void FrameData::add_cubic_bezier(float2 p0, float2 p1, float2 p2, float2 p3, Color color, float thickness) noexcept
{
  auto& cmd = _draw_cmds.emplace_back(DrawCmd::Type::add_cubic_bezier);
  cmd.data.add_cubic_bezier = { p0, p1, p2, p3, color, thickness };
}

void FrameData::_add_cubic_bezier(float2 p0, float2 p1, float2 p2, float2 p3, Color color, float thickness) noexcept
{
  if (_using_discard_shapes) color.a = 1.f;
  else if (color.a == 0) return;

  assert(_points.empty());
  _points.emplace_back(p0);
  path_bezier_cubic_curve_to_casteljau(p0, p1, p2, p3, curve_tessellation_tol, 0);
  add_poly_line(color, thickness, false);
}

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

void FrameData::_add_path_arc_to(float2 center, float2 p1, bool ccw) noexcept
{
  assert(!_points.empty());
  auto p0 = _points.back(); _points.pop_back();
  auto radius = length(p0 - center);
  if (radius < .5f)
  {
    _points.emplace_back(p1);
    return;
  }

  auto a0 = angle_of(center, p0);
  auto a1 = angle_of(center, p1);
  if (ccw) std::swap(a0, a1);
  path_arc_to(center, radius, a0, a1);
}

void FrameData::add_path_quad_bezier_to(float2 p1, float2 p2) noexcept
{
  auto& cmd = _draw_cmds.emplace_back(DrawCmd::Type::add_path_quad_bezier_to);
  cmd.data.add_path_quad_bezier_to = { p1, p2 };
}

void FrameData::_add_path_quad_bezier_to(float2 p1, float2 p2) noexcept
{
  assert(!_points.empty());
  path_bezier_quad_curve_to_casteljau(_points.back(), p1, p2, curve_tessellation_tol, 0);
}

void FrameData::add_path_cubic_bezier_to(float2 p1, float2 p2, float2 p3) noexcept
{
  auto& cmd = _draw_cmds.emplace_back(DrawCmd::Type::add_path_cubic_bezier_to);
  cmd.data.add_path_cubic_bezier_to = { p1, p2, p3 };
}

void FrameData::_add_path_cubic_bezier_to(float2 p1, float2 p2, float2 p3) noexcept
{
  assert(!_points.empty());
  path_bezier_cubic_curve_to_casteljau(_points.back(), p1, p2, p3, curve_tessellation_tol, 0);
}

void FrameData::path_end(bool close, Color color, float thickness) noexcept
{
  auto& cmd = _draw_cmds.emplace_back(DrawCmd::Type::path_end);
  cmd.data.path_end = { close, color, thickness };
}

void FrameData::union_beg() noexcept
{
  _draw_cmds.emplace_back(DrawCmd::Type::union_beg);
}

void FrameData::union_end(Color color, float thickness) noexcept
{
  auto& cmd = _draw_cmds.emplace_back(DrawCmd::Type::union_end);
  cmd.data.union_end = { color, thickness };
}

void FrameData::_path_end(bool close, Color color, float thickness) noexcept
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
    add_poly_line(color, thickness, close);
  else
  {
    if (is_convex(_points))
      add_convex_poly_filled(color);
    else
      add_concave_poly_filled(color);
  }
}

void FrameData::path_bezier_quad_curve_to_casteljau(float2 p0, float2 p1, float2 p2, float tess_tol, int level) noexcept
{
  auto dp  = p2 - p0;
  auto det = cross(p1 - p2, dp);
  if (det * det * 4.f < tess_tol * length_sq(dp))
    _points.emplace_back(p2);
  else if (level < 10)
  {
    auto p01  = ( p0 +  p1) * .5f;
    auto p12  = ( p1 +  p2) * .5f;
    auto p012 = (p01 + p12) * .5f;
    path_bezier_quad_curve_to_casteljau(p0, p01, p012, tess_tol, level + 1);
    path_bezier_quad_curve_to_casteljau(p012, p12, p2, tess_tol, level + 1);
  }
}

void FrameData::path_bezier_cubic_curve_to_casteljau(float2 p0, float2 p1, float2 p2, float2 p3, float tess_tol, int level) noexcept
{
  auto dp = p3 - p0;
  auto d2 = std::abs(cross(p1 - p3, dp));
  auto d3 = std::abs(cross(p2 - p3, dp));
  if (dot(float2(d2), float2(d3)) < tess_tol * length_sq(dp))
    _points.emplace_back(p3);
  else if (level < 10)
  {
    auto p01   = (  p0 +   p1) * .5f;
    auto p12   = (  p1 +   p2) * .5f;
    auto p23   = (  p2 +   p3) * .5f;
    auto p012  = ( p01 +  p12) * .5f;
    auto p123  = ( p12 +  p23) * .5f;
    auto p0123 = (p012 + p123) * .5f;
    path_bezier_cubic_curve_to_casteljau(p0, p01, p012, p0123, tess_tol, level + 1);
    path_bezier_cubic_curve_to_casteljau(p0123, p123, p23, p3, tess_tol, level + 1);
  }
}

void FrameData::_union_beg(uint& idx) noexcept
{
  using namespace Clipper2Lib;

  assert(!_build_mode.contains(BuildMode::uni));
  _build_mode.add(BuildMode::uni);

  auto subjects = PathsD{};

  auto add_current_path = [&]
  {
    if (_points.size() > 1 && _points.front() == _points.back())
      _points.pop_back();

    if (_points.size() < 3)
    {
      _points.clear();
      return;
    }

    auto& path = subjects.emplace_back();
    path.reserve(_points.size());
    for (auto const p : _points)
      path.emplace_back(static_cast<double>(p.x), static_cast<double>(p.y));

    _points.clear();
  };

  auto union_color     = Color{};
  auto union_thickness = 0.f;
  auto found_end       = false;

  while (++idx < _draw_cmds.size())
  {
    auto const& cmd = _draw_cmds[idx];
    switch (cmd.type)
    {
    case DrawCmd::Type::add_rect:
      assert(_points.empty());
      _points.emplace_back(cmd.data.add_rect.left_top);
      _points.emplace_back(cmd.data.add_rect.right_bottom.x, cmd.data.add_rect.left_top.y);
      _points.emplace_back(cmd.data.add_rect.right_bottom);
      _points.emplace_back(cmd.data.add_rect.left_top.x, cmd.data.add_rect.right_bottom.y);
      add_current_path();
      break;

    case DrawCmd::Type::add_triangle:
      assert(_points.empty());
      _points.emplace_back(cmd.data.add_triangle.p0);
      _points.emplace_back(cmd.data.add_triangle.p1);
      _points.emplace_back(cmd.data.add_triangle.p2);
      add_current_path();
      break;

    case DrawCmd::Type::add_circle:
      assert(_points.empty());
      path_arc_to(cmd.data.add_circle.center, cmd.data.add_circle.radius - .5f, 0, std::numbers::pi_v<float> * 2.f);
      if (!_points.empty())
        _points.pop_back();
      add_current_path();
      break;

    case DrawCmd::Type::path_begin:
      _points.clear();
      _path_begin(cmd.data.path_begin.p0);
      break;

    case DrawCmd::Type::add_path_line_to:
      if (!_points.empty())
        _add_path_line_to(cmd.data.add_path_line_to.p);
      break;

    case DrawCmd::Type::add_path_arc_to:
      if (!_points.empty())
        _add_path_arc_to(cmd.data.add_path_arc_to.center, cmd.data.add_path_arc_to.p1, cmd.data.add_path_arc_to.ccw);
      break;

    case DrawCmd::Type::add_path_quad_bezier_to:
      if (!_points.empty())
        _add_path_quad_bezier_to(cmd.data.add_path_quad_bezier_to.p1, cmd.data.add_path_quad_bezier_to.p2);
      break;

    case DrawCmd::Type::add_path_cubic_bezier_to:
      if (!_points.empty())
        _add_path_cubic_bezier_to(cmd.data.add_path_cubic_bezier_to.p1, cmd.data.add_path_cubic_bezier_to.p2, cmd.data.add_path_cubic_bezier_to.p3);
      break;

    case DrawCmd::Type::path_end:
      if (cmd.data.path_end.close)
        add_current_path();
      else
        _points.clear();
      break;

    case DrawCmd::Type::union_end:
      union_color     = cmd.data.union_end.color;
      union_thickness = cmd.data.union_end.thickness;
      found_end       = true;
      break;

    case DrawCmd::Type::discard_beg:
    case DrawCmd::Type::discard_end:
      assert(false && "discard operations cannot be nested or crossed");
      std::unreachable();

    default:
      _points.clear();
      break;
    }

    if (found_end)
      break;
  }

  _build_mode.remove(BuildMode::uni);
  _points.clear();
  if (!found_end || subjects.empty())
    return;

  if (_using_discard_shapes) union_color.a = 1.f;
  else if (union_color.a == 0) return;

  auto solution = Union(subjects, FillRule::NonZero, 2);
  for (auto const& path : solution)
  {
    if (path.size() < 3)
      continue;

    _points.reserve(path.size());
    for (auto const& pt : path)
      _points.emplace_back(static_cast<float>(pt.x), static_cast<float>(pt.y));

    if (union_thickness > 0)
      add_poly_line(union_color, union_thickness, true);
    else if (is_convex(_points))
      add_convex_poly_filled(union_color);
    else
      add_concave_poly_filled(union_color);
  }
}

void FrameData::add_convex_poly_filled(Color color) noexcept
{
  auto pt_cnt = static_cast<uint>(_points.size());
  assert(pt_cnt > 2);

  auto aa_width = g_ui_ctx.window()->scale();
  auto aa_col   = Color{ color.r, color.g, color.b, 0.f };

  auto [vertices, indices] = expand_beg(pt_cnt * 2, (pt_cnt - 2) * 3 + pt_cnt * 6);

  auto inner_idx = _vertex_beg;
  auto outer_idx = _vertex_beg + 1;
  for (auto i = 2; i < pt_cnt; ++i)
  {
    indices[0] = static_cast<uint16>(inner_idx);
    indices[1] = static_cast<uint16>(inner_idx + ((i - 1) << 1));
    indices[2] = static_cast<uint16>(inner_idx + (i << 1));
    indices += 3;
  }

  _normals.resize(pt_cnt);
  for (uint i0 = pt_cnt - 1, i1 = 0; i1 < pt_cnt; i0 = i1++)
  {
    auto dp = fast_normalize(_points[i1] - _points[i0]);
    _normals[i0] = { dp.y, -dp.x };
  }

  for (uint i0 = pt_cnt - 1, i1 = 0; i1 < pt_cnt; i0 = i1++)
  {
    auto dmp = fix_normal((_normals[i0] + _normals[i1]) * .5f) * aa_width * .5f;
    vertices[0] = { _points[i1] - dmp, {}, color  };
    vertices[1] = { _points[i1] + dmp, {}, aa_col };
    vertices += 2;

    indices[0] = static_cast<uint16>(inner_idx + (i1 << 1));
    indices[1] = static_cast<uint16>(inner_idx + (i0 << 1));
    indices[2] = static_cast<uint16>(outer_idx + (i0 << 1));
    indices[3] = static_cast<uint16>(outer_idx + (i0 << 1));
    indices[4] = static_cast<uint16>(outer_idx + (i1 << 1));
    indices[5] = static_cast<uint16>(inner_idx + (i1 << 1));
    indices += 6;
  }

  expand_end();
  _points.clear();
}

void FrameData::add_concave_poly_filled(Color color) noexcept
{
  auto pt_cnt = static_cast<uint>(_points.size());
  assert(pt_cnt > 2);

  uint triangle[3]{};
  auto const aa_size = g_ui_ctx.window()->scale();
  auto col_trans = Color{ color.r, color.g, color.b, 0 };

  auto const idx_cnt = (pt_cnt - 2) * 3 + pt_cnt * 6;
  auto const vtx_cnt = pt_cnt * 2;
  auto [vtx, idx] = expand_beg(vtx_cnt, idx_cnt);

  auto vtx_inner_idx = _vertex_beg;
  auto vtx_outer_idx = _vertex_beg + 1;

  _tmp_buf.resize((Triangulator::estimate_buf_size(pt_cnt) + sizeof(float2)) / sizeof(float2));
  auto triangulator = Triangulator{};
  triangulator.init(_points, _tmp_buf.data());
  while (triangulator.triangle_left() > 0)
  {
    triangulator.get_next_triangle(triangle);
    idx[0] = static_cast<uint16>(vtx_inner_idx + (triangle[0] << 1));
    idx[1] = static_cast<uint16>(vtx_inner_idx + (triangle[1] << 1));
    idx[2] = static_cast<uint16>(vtx_inner_idx + (triangle[2] << 1));
    idx += 3;
  }

  _tmp_buf.resize(pt_cnt);
  auto tmp_normals = _tmp_buf.data();
  for (int i0 = pt_cnt - 1, i1 = 0; i1 < static_cast<int>(pt_cnt); i0 = i1++)
  {
    auto dp = fast_normalize(_points[i1] - _points[i0]);
    tmp_normals[i0] = { dp.y, -dp.x };
  }

  for (int i0 = pt_cnt - 1, i1 = 0; i1 < static_cast<int>(pt_cnt); i0 = i1++)
  {
    auto dm = fix_normal((tmp_normals[i0] + tmp_normals[i1]) * .5f) * aa_size * .5f;

    vtx[0] = { _points[i1] - dm, {}, color     };
    vtx[1] = { _points[i1] + dm, {}, col_trans };
    vtx += 2;

    idx[0] = static_cast<uint16>(vtx_inner_idx + (i1 << 1));
    idx[1] = static_cast<uint16>(vtx_inner_idx + (i0 << 1));
    idx[2] = static_cast<uint16>(vtx_outer_idx + (i0 << 1));
    idx[3] = static_cast<uint16>(vtx_outer_idx + (i0 << 1));
    idx[4] = static_cast<uint16>(vtx_outer_idx + (i1 << 1));
    idx[5] = static_cast<uint16>(vtx_inner_idx + (i1 << 1));
    idx += 6;
  }

  expand_end();
  _points.clear();
}

auto FrameData::get_circle_segment_count(float radius) noexcept -> uint
{
  int const idx = static_cast<int>(radius + 0.999999f);
  if (idx >= 0 && idx < static_cast<int>(_circle_segment_counts.size()))
    return _circle_segment_counts[idx];
  return static_cast<uint>(calc_circle_segment_count(radius));
}

void FrameData::path_arc_to(float2 center, float radius, float min, float max) noexcept
{
  if (radius < .5f)
  {
    _points.emplace_back(center);
    return;
  }

  if (radius < arc_radius_cutoff)
  {
    auto const is_reverse = max < min;
    auto const min_sample_f = arc_sample_max * min / (std::numbers::pi * 2.f);
    auto const max_sample_f = arc_sample_max * max / (std::numbers::pi * 2.f);
    auto const min_sample = is_reverse ? std::floor(min_sample_f) : std::ceil(min_sample_f);
    auto const max_sample = is_reverse ? std::ceil(max_sample_f) : std::floor(max_sample_f);
    auto const mid_samples = is_reverse ? std::max(min_sample - max_sample, 0.0) : std::max(max_sample - min_sample, 0.0);
    auto const min_segment_angle = min_sample * std::numbers::pi * 2.f / arc_sample_max;
    auto const max_segment_angle = max_sample * std::numbers::pi * 2.f / arc_sample_max;
    auto const emit_start = std::abs(min_segment_angle - min) >= 1e-5f;
    auto const emit_end = std::abs(max - max_segment_angle) >= 1e-5f;

    _points.reserve(_points.size() + static_cast<size_t>(mid_samples) + 1 + emit_start + emit_end);
    if (emit_start) _points.emplace_back(center.x + std::cos(min) * radius, center.y + std::sin(min) * radius);
    if (mid_samples > 0) _path_arc_to(center, radius, static_cast<int>(min_sample), static_cast<int>(max_sample));
    if (emit_end) _points.emplace_back(center.x + std::cos(max) * radius, center.y + std::sin(max) * radius);
  }
  else
  {
    auto const arc_len = std::abs(max - min);
    auto const circle_segment_cnt = calc_circle_segment_count(radius);
    auto const arc_segment_cnt = std::max(std::ceil(circle_segment_cnt * arc_len / (std::numbers::pi * 2.f)), 1.0);
    _path_arc_to(center, radius, min, max, static_cast<int>(arc_segment_cnt));
  }
}

void FrameData::_path_arc_to(float2 center, float radius, int min, int max) noexcept
{
  auto step = std::clamp(arc_sample_max / static_cast<int>(get_circle_segment_count(radius)), 1, arc_table_size / 4);
  auto const sample_range = max - min;
  auto const next_step = step;

  auto samples = std::abs(sample_range) + 1;
  auto extra_max_sample = false;
  if (step > 1)
  {
    samples = std::abs(sample_range) / step + 1;
    auto const overstep = std::abs(sample_range) % step;
    if (overstep > 0)
    {
      extra_max_sample = true;
      ++samples;
      step -= (step - overstep) / 2;
    }
  }

  auto old_size = _points.size();
  _points.resize(old_size + samples);
  auto out_ptr = _points.data() + old_size;

  auto sample_index = min;
  if (sample_index < 0 || sample_index >= arc_sample_max)
  {
    sample_index %= arc_sample_max;
    if (sample_index < 0)
      sample_index += arc_sample_max;
  }

  if (max >= min)
  {
    for (auto a = min; a <= max; a += step, sample_index += step, step = next_step)
    {
      if (sample_index >= arc_sample_max)
        sample_index -= arc_sample_max;

      auto const s = _arc_vertices[sample_index];
      *out_ptr++ = { center.x + s.x * radius, center.y + s.y * radius };
    }
  }
  else
  {
    for (auto a = min; a >= max; a -= step, sample_index -= step, step = next_step)
    {
      if (sample_index < 0)
        sample_index += arc_sample_max;

      auto const s = _arc_vertices[sample_index];
      *out_ptr++ = { center.x + s.x * radius, center.y + s.y * radius };
    }
  }

  if (extra_max_sample)
  {
    auto normalized_max = max % arc_sample_max;
    if (normalized_max < 0)
      normalized_max += arc_sample_max;

    auto const s = _arc_vertices[normalized_max];
    *out_ptr++ = { center.x + s.x * radius, center.y + s.y * radius };
  }
}

void FrameData::_path_arc_to(float2 center, float radius, int min, int max, int segment_cnt) noexcept
{
  if (radius < .5f)
  {
    _points.emplace_back(center);
    return;
  }

  _points.reserve(_points.size() + segment_cnt + 1);
  auto m = max - min;
  for (auto i = 0; i <= segment_cnt; ++i)
  {
    auto const a = min + (static_cast<float>(i) / segment_cnt) * m;
    _points.emplace_back(center.x + std::cos(a) * radius, center.y + std::sin(a) * radius);
  }
}

void FrameData::add_poly_line(Color color, float thickness, bool is_closed) noexcept
{
  auto pt_cnt = static_cast<uint>(_points.size());
  assert(pt_cnt >= 2);

  auto const count      = is_closed ? pt_cnt : pt_cnt - 1;
  auto const aa_size    = g_ui_ctx.window()->scale();
  auto const thick_line = thickness > aa_size;
  auto col_trans = Color{ color.r, color.g, color.b, 0 };

  thickness = std::max(thickness, 1.f);

  auto const idx_cnt = thick_line ? count * 18 : count * 12;
  auto const vtx_cnt = thick_line ? pt_cnt * 4 : pt_cnt * 3;

  auto [vtx, idx] = expand_beg(vtx_cnt, idx_cnt);

  _tmp_buf.clear();
  _tmp_buf.resize(pt_cnt * (thick_line ? 5 : 3));
  auto* tmp_normals = _tmp_buf.data();
  auto* tmp_points  = tmp_normals + pt_cnt;

  for (auto i1 = 0u; i1 < count; ++i1)
  {
    auto const i2 = (i1 + 1) == pt_cnt ? 0 : i1 + 1;
    auto dp = fast_normalize(_points[i2] - _points[i1]);
    tmp_normals[i1] = { dp.y, -dp.x };
  }
  if (!is_closed) tmp_normals[pt_cnt - 1] = tmp_normals[pt_cnt - 2];

  if (thick_line)
  {
    auto const half_inner_thickness = (thickness - aa_size) * .5f;

    if (!is_closed)
    {
      auto const last = pt_cnt - 1;
      tmp_points[0] = _points[0] + tmp_normals[0] * (half_inner_thickness + aa_size);
      tmp_points[1] = _points[0] + tmp_normals[0] * half_inner_thickness;
      tmp_points[2] = _points[0] - tmp_normals[0] * half_inner_thickness;
      tmp_points[3] = _points[0] - tmp_normals[0] * (half_inner_thickness + aa_size);
      tmp_points[last * 4 + 0] = _points[last] + tmp_normals[last] * (half_inner_thickness + aa_size);
      tmp_points[last * 4 + 1] = _points[last] + tmp_normals[last] * half_inner_thickness;
      tmp_points[last * 4 + 2] = _points[last] - tmp_normals[last] * half_inner_thickness;
      tmp_points[last * 4 + 3] = _points[last] - tmp_normals[last] * (half_inner_thickness + aa_size);
    }

    auto idx1 = _vertex_beg;
    for (auto i1 = 0u; i1 < count; ++i1)
    {
      auto const i2   = (i1 + 1) == pt_cnt ? 0 : i1 + 1;
      auto const idx2 = ((i1 + 1) == pt_cnt) ? _vertex_beg : (idx1 + 4);

      auto dm = fix_normal((tmp_normals[i1] + tmp_normals[i2]) * .5f);
      auto dm_out = dm * (half_inner_thickness + aa_size);
      auto dm_in  = dm * half_inner_thickness;

      auto* out_vtx = &tmp_points[i2 * 4];
      out_vtx[0] = _points[i2] + dm_out;
      out_vtx[1] = _points[i2] + dm_in;
      out_vtx[2] = _points[i2] - dm_in;
      out_vtx[3] = _points[i2] - dm_out;

      uint vals[] = { idx2 + 1, idx1 + 1, idx1 + 2, idx1 + 2, idx2 + 2, idx2 + 1, idx2 + 1, idx1 + 1, idx1 + 0, idx1 + 0, idx2 + 0, idx2 + 1, idx2 + 2, idx1 + 2, idx1 + 3, idx1 + 3, idx2 + 3, idx2 + 2 };
      for (auto v : vals) *idx++ = static_cast<uint16>(v);

      idx1 = idx2;
    }

    for (auto i = 0u; i < pt_cnt; ++i)
    {
      vtx[0] = { tmp_points[i * 4], {}, col_trans };
      vtx[1] = { tmp_points[i * 4 + 1], {}, color };
      vtx[2] = { tmp_points[i * 4 + 2], {}, color };
      vtx[3] = { tmp_points[i * 4 + 3], {}, col_trans };
      vtx += 4;
    }
  }
  else
  {
    auto const half_draw_size = aa_size;
    if (!is_closed)
    {
      tmp_points[0] = _points[0] + tmp_normals[0] * half_draw_size;
      tmp_points[1] = _points[0] - tmp_normals[0] * half_draw_size;
      tmp_points[(pt_cnt - 1) * 2 + 0] = _points[pt_cnt - 1] + tmp_normals[pt_cnt - 1] * half_draw_size;
      tmp_points[(pt_cnt - 1) * 2 + 1] = _points[pt_cnt - 1] - tmp_normals[pt_cnt - 1] * half_draw_size;
    }

    auto idx1 = _vertex_beg;
    for (auto i1 = 0u; i1 < count; ++i1)
    {
      auto const i2   = (i1 + 1) == pt_cnt ? 0 : i1 + 1;
      auto const idx2 = ((i1 + 1) == pt_cnt) ? _vertex_beg : (idx1 + 3);

      auto dm = fix_normal((tmp_normals[i1] + tmp_normals[i2]) * .5f) * half_draw_size;
      auto* out_vtx = &tmp_points[i2 * 2];
      out_vtx[0] = _points[i2] + dm;
      out_vtx[1] = _points[i2] - dm;

      uint vals[] = { idx2 + 0, idx1 + 0, idx1 + 2, idx1 + 2, idx2 + 2, idx2 + 0, idx2 + 1, idx1 + 1, idx1 + 0, idx1 + 0, idx2 + 0, idx2 + 1 };
      for (auto v : vals) *idx++ = static_cast<uint16>(v);

      idx1 = idx2;
    }

    for (auto i = 0u; i < pt_cnt; ++i)
    {
      vtx[0] = { _points[i], {}, color };
      vtx[1] = { tmp_points[i * 2 + 0], {}, col_trans };
      vtx[2] = { tmp_points[i * 2 + 1], {}, col_trans };
      vtx += 3;
    }
  }

  expand_end();
  _points.clear();
}

auto FrameData::get_vertices_bound_rect(uint vtx_beg, uint vtx_cnt) const noexcept -> Rect
{
  assert(vtx_beg + vtx_cnt <= _vertices.size());

  auto rc = Rect{};
  for (auto i = 0; i < vtx_cnt; ++i)
  {
    auto const& vtx = _vertices[vtx_beg + i];
    rc.left   = std::min(rc.left,   std::floor(vtx.pos.x));
    rc.top    = std::min(rc.top,    std::floor(vtx.pos.y));
    rc.right  = std::max(rc.right,  std::ceil(vtx.pos.x));
    rc.bottom = std::max(rc.bottom, std::ceil(vtx.pos.y));
  }

  return rc;
}

void FrameData::push_render_cmd(RenderCmdType type, ImageHandle image_handle) noexcept
{
  if (auto res = static_cast<uint>(_indices.size() - _draw_index_beg))
  {
    if (type == RenderCmdType::ui)
      _render_cmd_rect_idxs.emplace_back(_render_cmds.size());

    auto& cmd = _render_cmds.emplace_back();
    cmd.type            = type;
    cmd.ui.idx_beg      = _draw_index_beg;
    cmd.ui.idx_size     = res;
    cmd.ui.image_handle = image_handle;

    _draw_index_beg = static_cast<uint>(_indices.size());

    renderer::g_img_mgr[image_handle].graphics_will_use();
  }
}

void FrameData::push_render_cmd_clear_rect(RenderCmdType type, std::optional<Rect> rect) noexcept
{
  auto& cmd = _render_cmds.emplace_back();
  cmd.type       = type;
  cmd.clear_rect = rect;
}

void FrameData::add_scissor_rect(Rect rect) noexcept
{
  auto& cmd = _draw_cmds.emplace_back(DrawCmd::Type::add_scissor_rect);
  cmd.data.add_scissor_rect = { rect };
}

void FrameData::_add_scissor_rect(Rect rect) noexcept
{
  push_render_cmd(RenderCmdType::ui);

  for (auto idx : _render_cmd_rect_idxs)
    _render_cmds[idx].ui.scissor_rect = rect;
  _render_cmd_rect_idxs.clear();
}

void FrameData::add_image(ImageHandle handle, float2 left_top, float2 right_bottom, uint8 alpha, std::span<float2> uvs) noexcept
{
  assert(uvs.size() == 4);
  auto& cmd = _draw_cmds.emplace_back(DrawCmd::Type::add_image);
  cmd.data.add_image = { handle, left_top, right_bottom, alpha, uvs[0], uvs[1], uvs[2], uvs[3] };
}

void FrameData::_add_image(ImageHandle handle, float2 left_top, float2 right_bottom, uint8 alpha, float2 uv0, float2 uv1, float2 uv2, float2 uv3) noexcept
{
  assert(!_using_discard_shapes);
  if (alpha == 0) return;

  auto type = _build_mode.contains(BuildMode::discard) ? RenderCmdType::discard_draw_composite : RenderCmdType::ui;
  push_render_cmd(type);

  auto col = Color{ 1, 1, 1, static_cast<float>(alpha) / 255 };
  auto [vertices, indices] = expand_beg(4, 6);
  vertices[0] = { left_top, uv0, col };
  vertices[1] = { { right_bottom.x, left_top.y }, uv1, col };
  vertices[2] = { right_bottom, uv2, col };
  vertices[3] = { { left_top.x, right_bottom.y }, uv3, col };
  indices[0]  = static_cast<uint16>(_vertex_beg + 0);
  indices[1]  = static_cast<uint16>(_vertex_beg + 1);
  indices[2]  = static_cast<uint16>(_vertex_beg + 2);
  indices[3]  = static_cast<uint16>(_vertex_beg + 0);
  indices[4]  = static_cast<uint16>(_vertex_beg + 2);
  indices[5]  = static_cast<uint16>(_vertex_beg + 3);
  expand_end();

  push_render_cmd(type, handle);
}

void FrameData::discard_beg(std::function<void()> func) noexcept
{
  _discard_beg_idx = _draw_cmds.size();
  _draw_cmds.emplace_back(DrawCmd::Type::discard_beg);
  auto beg = _draw_cmds.size();
  func();
  _draw_cmds[_discard_beg_idx].data.discard_beg.count = static_cast<uint>(_draw_cmds.size() - beg);
}

void FrameData::_discard_beg(uint count, uint& idx) noexcept
{
  assert(!_build_mode.contains(BuildMode::discard));
  assert(!_build_mode.contains(BuildMode::uni));
  _build_mode.add(BuildMode::discard);

  push_render_cmd(RenderCmdType::ui);

  _discard_vtx_beg      = _vertex_beg;
  _using_discard_shapes = true;
  for (auto i = 0u; i < count; ++i) build_render_cmd(_draw_cmds[++idx], idx);
  _using_discard_shapes = false;

  push_render_cmd_clear_rect(RenderCmdType::clear_discard_image, get_vertices_bound_rect(_discard_vtx_beg));
  push_render_cmd(RenderCmdType::discard_write);

  _clear_composite_image_cmd_idx = _render_cmds.size();
  push_render_cmd_clear_rect(RenderCmdType::clear_composite_image);

  _build_mode.add(BuildMode::discard);
  _discard_vtx_beg = _vertex_beg;
}

void FrameData::discard_end() noexcept
{
  // if not have any discard targets, remove the discard operation
  if (_draw_cmds[_discard_beg_idx].data.discard_beg.count + _discard_beg_idx == _draw_cmds.size() - 1)
    _draw_cmds.erase(_draw_cmds.begin() + _discard_beg_idx, _draw_cmds.end());
  else
    _draw_cmds.emplace_back(DrawCmd::Type::discard_end);
}

void FrameData::_discard_end() noexcept
{
  assert(_build_mode.contains(BuildMode::discard) && _vertices.size() > _discard_vtx_beg);

  _build_mode.remove(BuildMode::discard);

  auto rc = get_vertices_bound_rect(_discard_vtx_beg);
  _render_cmds[_clear_composite_image_cmd_idx].clear_rect = rc;
  push_render_cmd(RenderCmdType::discard_draw_composite);
  add_rect({ rc.left, rc.top }, { rc.right, rc.bottom });
  push_render_cmd(RenderCmdType::discard_composite);

  auto wnd = g_ui_ctx.window();
  _render_cmds.back().ui.scissor_rect = wnd->is_resizing() ? wnd->rect() : wnd->content_rect(); 
}

void FrameData::build_render_cmd(DrawCmd const& cmd, uint& idx) noexcept
{
  using Type = DrawCmd::Type;

  switch (cmd.type)
  {
  case Type::add_rect:
    _add_rect(cmd.data.add_rect.left_top, cmd.data.add_rect.right_bottom, cmd.data.add_rect.color, cmd.data.add_rect.thickness);
    break;

  case Type::add_triangle:
    _add_triangle(cmd.data.add_triangle.p0, cmd.data.add_triangle.p1, cmd.data.add_triangle.p2, cmd.data.add_triangle.color, cmd.data.add_triangle.thickness);
    break;

  case Type::add_circle:
    _add_circle(cmd.data.add_circle.center, cmd.data.add_circle.radius, cmd.data.add_circle.color, cmd.data.add_circle.thickness);
    break;

  case Type::add_line:
    _add_line(cmd.data.add_line.p0, cmd.data.add_line.p1, cmd.data.add_line.color, cmd.data.add_line.thickness);
    break;

  case Type::add_arc:
    _add_arc(cmd.data.add_arc.center, cmd.data.add_arc.p0, cmd.data.add_arc.p1, cmd.data.add_arc.color, cmd.data.add_arc.thickness);
    break;

  case Type::add_quad_bezier:
    _add_quad_bezier(cmd.data.add_quad_bezier.p0, cmd.data.add_quad_bezier.p1, cmd.data.add_quad_bezier.p2, cmd.data.add_quad_bezier.color, cmd.data.add_quad_bezier.thickness);
    break;

  case Type::add_cubic_bezier:
    _add_cubic_bezier(cmd.data.add_cubic_bezier.p0, cmd.data.add_cubic_bezier.p1, cmd.data.add_cubic_bezier.p2, cmd.data.add_cubic_bezier.p3, cmd.data.add_cubic_bezier.color, cmd.data.add_cubic_bezier.thickness);
    break;

  case Type::add_image:
    _add_image(
      cmd.data.add_image.handle,
      cmd.data.add_image.left_top,
      cmd.data.add_image.right_bottom,
      cmd.data.add_image.alpha,
      cmd.data.add_image.uv0,
      cmd.data.add_image.uv1,
      cmd.data.add_image.uv2,
      cmd.data.add_image.uv3
    );
    break;

  case Type::path_begin:
    _path_begin(cmd.data.path_begin.p0);
    break;

  case Type::add_path_line_to:
    _add_path_line_to(cmd.data.add_path_line_to.p);
    break;

  case Type::add_path_arc_to:
    _add_path_arc_to(cmd.data.add_path_arc_to.center, cmd.data.add_path_arc_to.p1, cmd.data.add_path_arc_to.ccw);
    break;

  case Type::add_path_quad_bezier_to:
    _add_path_quad_bezier_to(cmd.data.add_path_quad_bezier_to.p1, cmd.data.add_path_quad_bezier_to.p2);
    break;

  case Type::add_path_cubic_bezier_to:
    _add_path_cubic_bezier_to(cmd.data.add_path_cubic_bezier_to.p1, cmd.data.add_path_cubic_bezier_to.p2, cmd.data.add_path_cubic_bezier_to.p3);
    break;

  case Type::path_end:
    _path_end(cmd.data.path_end.close, cmd.data.path_end.color, cmd.data.path_end.thickness);
    break;

  case Type::union_beg:
    _union_beg(idx);
    break;

  case Type::union_end:
    break;

  case Type::add_scissor_rect:
    _add_scissor_rect(cmd.data.add_scissor_rect.rect);
    break;

  case Type::discard_beg:
    _discard_beg(cmd.data.discard_beg.count, idx);
    break;

  case Type::discard_end:
    _discard_end();
    break;
  }
}

void FrameData::build_render_cmds() noexcept
{
  for (auto idx = 0u; idx < _draw_cmds.size(); ++idx)
    build_render_cmd(_draw_cmds[idx], idx);
}

}
