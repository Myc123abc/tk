#include "frame_data.hpp"
#include "ui_context.hpp"
#include "triangulator.hpp"

#include <ranges>
#include <numbers>

using namespace tk;

namespace {

auto sqrt(float x) noexcept
{
  return _mm_cvtss_f32(_mm_rsqrt_ss(_mm_set_ss(x)));
}

auto normalize(vec2 p) noexcept
{
  auto d2 = dot(p, p);
  if (d2 > .0f)
  {
    auto inv_len = sqrt(d2);
    return p * inv_len;
  }
  return vec2{};
}

auto fix_normal(vec2 p) noexcept
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
  return vec2{};
}

auto round_up_to_even(int x) noexcept
{
  return ((x + 1) / 2) * 2;
}

auto calc_circle_radius(float cnt, float max_error) noexcept
{
  return max_error / (1 - std::cos(std::numbers::pi_v<float> / std::max(cnt, std::numbers::pi_v<float>)));
}

auto is_convex(std::span<vec2> p) noexcept
{
  auto const n = p.size();
  if (n < 3)
    return false;

  auto sign = 0.0f;

  for (auto i = 0; i < n; ++i)
  {
    auto const a = p[i];
    auto const b = p[(i + 1) % n];
    auto const c = p[(i + 2) % n];

    auto ab = b - a;
    auto bc = c - b;

    auto cross = ab.x * bc.y - ab.y * bc.x;

    if (std::abs(cross) < 1e-6f)
      continue;

    if (sign == .0f)
      sign = cross;
    else if ((cross > 0.0f) != (sign > 0.0f))
      return false;
  }

  return true;
}

}

namespace tk::ui {

void FrameData::init() noexcept
{
  // cache circle segment counts
  _circle_segment_counts[0] = arc_sample_max;
  for (auto i : std::views::iota(1u, _circle_segment_counts.size()))
    _circle_segment_counts[i] = calc_circle_segment_count(i);

  // calculate arc vertices
  for (auto i : std::views::iota(0u, _arc_vertices.size()))
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

void FrameData::add_rect(vec2 left_top, vec2 right_bottom, Color color, float thickness) noexcept
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

void FrameData::add_rect(vec2 left_top, vec2 right_bottom, Color color) noexcept
{
  auto [vertices, indices] = expand_beg(4, 6);

  vertices[0] = { left_top, {}, color };
  vertices[1] = { { right_bottom.x, left_top.y }, {}, color };
  vertices[2] = { right_bottom, {}, color };
  vertices[3] = { { left_top.x, right_bottom.y }, {}, color };
  indices[0]  = _vertex_beg + 0;
  indices[1]  = _vertex_beg + 1;
  indices[2]  = _vertex_beg + 2;
  indices[3]  = _vertex_beg + 0;
  indices[4]  = _vertex_beg + 2;
  indices[5]  = _vertex_beg + 3;

  expand_end();
}

void FrameData::add_triangle(vec2 p0, vec2 p1, vec2 p2, Color color, float thickness) noexcept
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

void FrameData::add_circle(vec2 center, float radius, Color color, float thickness) noexcept
{
  if (_using_discard_shapes) color.a = 1.f;
  else if (color.a == 0) return;

  assert(_points.empty());
  _path_arc_to(center, radius - .5f, 0, arc_sample_max);
  _points.pop_back();
  if (thickness > 0)
    add_poly_line(color, thickness, true);
  else
    add_convex_poly_filled(color);
}

void FrameData::add_line(vec2 p0, vec2 p1, Color color, float thickness) noexcept
{
  if (_using_discard_shapes) color.a = 1.f;
  else if (color.a == 0) return;

  assert(_points.empty());
  _points.emplace_back(p0);
  _points.emplace_back(p1);
  add_poly_line(color, thickness, false);
}

void FrameData::add_bezier_quad(vec2 p0, vec2 p1, vec2 p2, Color color, float thickness) noexcept
{
  if (_using_discard_shapes) color.a = 1.f;
  else if (color.a == 0) return;

  assert(_points.empty());
  _points.emplace_back(p0);
  path_bezier_quad_curve_to_casteljau(p0, p1, p2, curve_tessellation_tol, 0);
  add_poly_line(color, thickness, false);
}

void FrameData::add_bezier_cubic(vec2 p0, vec2 p1, vec2 p2, vec2 p3, Color color, float thickness) noexcept
{
  if (_using_discard_shapes) color.a = 1.f;
  else if (color.a == 0) return;

  assert(_points.empty());
  _points.emplace_back(p0);
  path_bezier_cubic_curve_to_casteljau(p0, p1, p2, p3, curve_tessellation_tol, 0);
  add_poly_line(color, thickness, false);
}

void FrameData::path_end(Color color, float thickness, bool is_closed) noexcept
{
  if (_using_discard_shapes) color.a = 1.f;
  else if (color.a == 0)
  {
    _points.clear();
    return;
  }

  if (thickness > 0)
    add_poly_line(color, thickness, is_closed);
  else
  {
    if (is_convex(_points))
      add_convex_poly_filled(color);
    else
      add_concave_poly_filled(color);
  }
}

void FrameData::path_bezier_quad_curve_to_casteljau(vec2 p0, vec2 p1, vec2 p2, float tess_tol, int level) noexcept
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

void FrameData::path_bezier_cubic_curve_to_casteljau(vec2 p0, vec2 p1, vec2 p2, vec2 p3, float tess_tol, int level) noexcept
{
  auto dp = p3 - p0;
  auto d2 = cross(p1 - p3, dp);
  auto d3 = cross(p2 - p3, dp);
  d2 = d2 >= 0 ? d2 : -d2;
  d3 = d3 >= 0 ? d3 : -d3;
  if (dot(vec2(d2), vec2(d3)) < tess_tol * length_sq(dp))
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

void FrameData::add_convex_poly_filled(Color color) noexcept
{
  auto pt_cnt = _points.size();
  assert(pt_cnt > 2);

  if (color.a == 0)
  {
    _points.clear();
    return;
  }

  auto aa_width = g_ui_ctx.window()->scale();
  auto aa_col   = Color{ color.r, color.g, color.b, 0.f };

  auto [vertices, indices] = expand_beg(pt_cnt * 2, (pt_cnt - 2) * 3 + pt_cnt * 6);

  auto inner_idx = _vertex_beg;
  auto outer_idx = _vertex_beg + 1;
  for (auto i : std::views::iota(2u, pt_cnt))
  {
    indices[0] = inner_idx;
    indices[1] = inner_idx + ((i - 1) << 1);
    indices[2] = inner_idx + (i << 1);
    indices += 3;
  }

  _normals.resize(pt_cnt);
  for (uint32_t i0 = pt_cnt - 1, i1 = 0; i1 < pt_cnt; i0 = i1++)
  {
    auto dp = normalize(_points[i1] - _points[i0]);
    _normals[i0] = { dp.y, -dp.x };
  }

  for (uint32_t i0 = pt_cnt - 1, i1 = 0; i1 < pt_cnt; i0 = i1++)
  {
    auto dmp = fix_normal((_normals[i0] + _normals[i1]) * .5f) * aa_width * .5f;
    vertices[0] = { _points[i1] - dmp, {}, color  };
    vertices[1] = { _points[i1] + dmp, {}, aa_col };
    vertices += 2;

    indices[0] = inner_idx + (i1 << 1);
    indices[1] = inner_idx + (i0 << 1);
    indices[2] = outer_idx + (i0 << 1);
    indices[3] = outer_idx + (i0 << 1);
    indices[4] = outer_idx + (i1 << 1);
    indices[5] = inner_idx + (i1 << 1);
    indices += 6;
  }

  expand_end();
  _points.clear();
}

void FrameData::add_concave_poly_filled(Color color) noexcept
{
  auto pt_cnt = _points.size();
  assert(pt_cnt > 2);

  if (color.a == 0)
  {
    _points.clear();
    return;
  }

  uint32_t triangle[3]{};

  auto const aa_size = g_ui_ctx.window()->scale();

  auto col_trans = Color{ color.r, color.g, color.b, 0 };

  auto const idx_cnt = (pt_cnt - 2) * 3 + pt_cnt * 6;
  auto const vtx_cnt = pt_cnt * 2;

  auto [vtx, idx] = expand_beg(vtx_cnt, idx_cnt);

  auto vtx_inner_idx = _vertex_beg;
  auto vtx_outer_idx = _vertex_beg + 1;

  _tmp_buf.resize((Triangulator::estimate_buf_size(pt_cnt) + sizeof(vec2)) / sizeof(vec2));
  auto triangulator = Triangulator{};
  triangulator.init(_points, _tmp_buf.data());
  while (triangulator.triangle_left() > 0)
  {
    triangulator.get_next_triangle(triangle);
    idx[0] = vtx_inner_idx + (triangle[0] << 1);
    idx[1] = vtx_inner_idx + (triangle[1] << 1);
    idx[2] = vtx_inner_idx + (triangle[2] << 1);
    idx += 3;
  }

  _tmp_buf.resize(pt_cnt);
  auto tmp_normals = _tmp_buf.data();
  for (int i0 = pt_cnt - 1, i1 = 0; i1 < pt_cnt; i0 = i1++)
  {
    auto const p0 = _points[i0];
    auto const p1 = _points[i1];
    auto dp = p1 - p0;
    dp = normalize(dp);
    tmp_normals[i0] = { dp.y, -dp.x };
  }

  for (int i0 = pt_cnt - 1, i1 = 0; i1 < pt_cnt; i0 = i1++)
  {
    auto p0 = tmp_normals[i0];
    auto p1 = tmp_normals[i1];
    auto dm = (p0 + p1) * .5f;
    dm = fix_normal(dm);
    dm *= aa_size * .5f;

    vtx[0] = { _points[i1] - dm, {}, color     };
    vtx[1] = { _points[i1] + dm, {}, col_trans };
    vtx += 2;

    idx[0] = vtx_inner_idx + (i1 << 1);
    idx[1] = vtx_inner_idx + (i0 << 1);
    idx[2] = vtx_outer_idx + (i0 << 1);
    idx[3] = vtx_outer_idx + (i0 << 1);
    idx[4] = vtx_outer_idx + (i1 << 1);
    idx[5] = vtx_inner_idx + (i1 << 1);
    idx += 6;
  }

  expand_end();
  _points.clear();
}

auto FrameData::get_circle_segment_count(float radius) noexcept -> uint32_t
{
  int const idx = radius + 0.999999f;
  if (idx >= 0 && idx < _circle_segment_counts.size())
    return _circle_segment_counts[idx];
  return calc_circle_segment_count(radius);
}

void FrameData::path_arc_to(vec2 center, float radius, float min, float max) noexcept
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

    _points.reserve(_points.size() + mid_samples + 1 + emit_start + emit_end);
    if (emit_start) _points.emplace_back(center.x + std::cos(min) * radius, center.y + sin(min) * radius);
    if (mid_samples > 0) _path_arc_to(center, radius, min_sample, max_sample);
    if (emit_end) _points.emplace_back(center.x + std::cos(max) * radius, center.y + sin(max) * radius);
  }
  else
  {
    auto const arc_len = std::abs(max - min);
    auto const circle_segement_cnt = calc_circle_segment_count(radius);
    auto const arc_segment_cnt     = std::max(std::ceil(circle_segement_cnt * arc_len / (std::numbers::pi * 2.f)), 1.0);
    _path_arc_to(center, radius, min, max, arc_segment_cnt);
  }
}

void FrameData::_path_arc_to(vec2 center, float radius, int min, int max) noexcept
{
  auto step = std::clamp(arc_sample_max / static_cast<int>(get_circle_segment_count(radius)),
    1, arc_table_size / 4);
  
  auto const sample_range = max - min;
  auto const next_step    = step;

  auto samples          = sample_range + 1;
  auto extra_max_sample = false;
  if (step > 1)
  {
    samples             = sample_range / step + 1;
    auto const overstep = sample_range % step;
    if (overstep > 0)
    {
      extra_max_sample = true;
      ++samples;

      if (sample_range > 0)
        step -= (step - overstep) / 2;
    }
  }

  assert(_points.empty());
  _points.resize(samples);
  auto out_ptr = _points.data() + (_points.size() - samples);

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
      out_ptr->x = center.x + s.x * radius;
      out_ptr->y = center.y + s.y * radius;
      ++out_ptr;
    }
  }
  else
  {
    for (auto a = min; a >= max; a -= step, sample_index -= step, step = next_step)
    {
      if (sample_index < 0)
        sample_index += arc_sample_max;

      auto const s = _arc_vertices[sample_index];
      out_ptr->x = center.x + s.x * radius;
      out_ptr->y = center.y + s.y * radius;
      ++out_ptr;
    }
  }

  if (extra_max_sample)
  {
    auto normalized_max = max % arc_sample_max;
    if (normalized_max < 0)
      normalized_max += arc_sample_max;

    auto const s = _arc_vertices[normalized_max];
    out_ptr->x = center.x + s.x * radius;
    out_ptr->y = center.y + s.y * radius;
    ++out_ptr;
  }

  assert(_points.data() + _points.size() == out_ptr);
}

void FrameData::_path_arc_to(vec2 center, float radius, int min, int max, int segment_cnt) noexcept
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

void FrameData::path_bezier_quad_to(vec2 p1, vec2 p2) noexcept
{
  assert(!_points.empty());
  path_bezier_quad_curve_to_casteljau(_points.back(), p1, p2, curve_tessellation_tol, 0);
}

void FrameData::path_bezier_cubic_to(vec2 p1, vec2 p2, vec2 p3) noexcept
{
  assert(!_points.empty());
  path_bezier_cubic_curve_to_casteljau(_points.back(), p1, p2, p3, curve_tessellation_tol, 0);
}

void FrameData::add_poly_line(Color color, float thickness, bool is_closed) noexcept
{
  auto pt_cnt = _points.size();
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

  for (auto i1 = 0; i1 < count; ++i1)
  {
    auto const i2 = (i1 + 1) == pt_cnt ? 0 : i1 + 1;
    auto dp = _points[i2] - _points[i1];
    dp = normalize(dp);
    tmp_normals[i1] = { dp.y, -dp.x };
  }
  if (!is_closed) tmp_normals[pt_cnt - 1] = tmp_normals[pt_cnt - 2];

  if (thick_line)
  {
    auto const half_inner_thickness = (thickness - aa_size) * .5f;

    if (!is_closed)
    {
      auto const points_last = pt_cnt - 1;
      tmp_points[0] = _points[0] + tmp_normals[0] * (half_inner_thickness + aa_size);
      tmp_points[1] = _points[0] + tmp_normals[0] * (half_inner_thickness);
      tmp_points[2] = _points[0] - tmp_normals[0] * (half_inner_thickness);
      tmp_points[3] = _points[0] - tmp_normals[0] * (half_inner_thickness + aa_size);
      tmp_points[points_last * 4 + 0] = _points[points_last] + tmp_normals[points_last] * (half_inner_thickness + aa_size);
      tmp_points[points_last * 4 + 1] = _points[points_last] + tmp_normals[points_last] * (half_inner_thickness);
      tmp_points[points_last * 4 + 2] = _points[points_last] - tmp_normals[points_last] * (half_inner_thickness);
      tmp_points[points_last * 4 + 3] = _points[points_last] - tmp_normals[points_last] * (half_inner_thickness + aa_size);
    }

    auto idx1 = _vertex_beg;
    for (auto i1 = 0; i1 < count; ++i1)
    {
      auto const i2   = (i1 + 1) == pt_cnt ? 0 : i1 + 1;
      auto const idx2 = ((i1 + 1) == pt_cnt) ? _vertex_beg : (idx1 + 4);

      auto dm = (tmp_normals[i1] + tmp_normals[i2]) * .5f;
      dm = fix_normal(dm);
      auto dm_out = dm * (half_inner_thickness + aa_size);
      auto dm_in  = dm * half_inner_thickness;

      auto* out_vtx = &tmp_points[i2 * 4];
      out_vtx[0] = _points[i2] + dm_out;
      out_vtx[1] = _points[i2] + dm_in;
      out_vtx[2] = _points[i2] - dm_in;
      out_vtx[3] = _points[i2] - dm_out;

      idx[ 0] = idx2 + 1; idx[ 1] = idx1 + 1; idx[ 2] = idx1 + 2;
      idx[ 3] = idx1 + 2; idx[ 4] = idx2 + 2; idx[ 5] = idx2 + 1;
      idx[ 6] = idx2 + 1; idx[ 7] = idx1 + 1; idx[ 8] = idx1 + 0;
      idx[ 9] = idx1 + 0; idx[10] = idx2 + 0; idx[11] = idx2 + 1;
      idx[12] = idx2 + 2; idx[13] = idx1 + 2; idx[14] = idx1 + 3;
      idx[15] = idx1 + 3; idx[16] = idx2 + 3; idx[17] = idx2 + 2;
      idx += 18;

      idx1 = idx2;
    }

    for (auto i = 0; i < pt_cnt; ++i)
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
    for (auto i1 = 0; i1 < count; ++i1)
    {
      auto const i2   = (i1 + 1) == pt_cnt ? 0 : i1 + 1;
      auto const idx2 = ((i1 + 1) == pt_cnt) ? _vertex_beg : (idx1 + 3);

      auto dm = (tmp_normals[i1] + tmp_normals[i2]) * .5f;
      dm = fix_normal(dm);
      dm *= half_draw_size;

      auto* out_vtx = &tmp_points[i2 * 2];
      out_vtx[0] = _points[i2] + dm;
      out_vtx[1] = _points[i2] - dm;

      idx[0] = idx2 + 0; idx[ 1] = idx1 + 0; idx[ 2] = idx1 + 2;
      idx[3] = idx1 + 2; idx[ 4] = idx2 + 2; idx[ 5] = idx2 + 0;
      idx[6] = idx2 + 1; idx[ 7] = idx1 + 1; idx[ 8] = idx1 + 0;
      idx[9] = idx1 + 0; idx[10] = idx2 + 0; idx[11] = idx2 + 1;
      idx += 12;

      idx1 = idx2;
    }

    for (auto i = 0; i < pt_cnt; ++i)
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

auto FrameData::get_rect(uint32_t vtx_beg, uint32_t vtx_cnt) const noexcept -> RECT
{
  assert(vtx_beg + vtx_cnt <= _vertices.size());

  auto rc = RECT{ LONG_MAX, LONG_MAX, LONG_MIN, LONG_MIN };

  for (auto i : std::views::iota(0u, vtx_cnt))
  {
    auto const& vtx = _vertices[vtx_beg + i];
    rc.left   = std::min(rc.left,   static_cast<LONG>(std::floor(vtx.pos.x)));
    rc.top    = std::min(rc.top,    static_cast<LONG>(std::floor(vtx.pos.y)));
    rc.right  = std::max(rc.right,  static_cast<LONG>(std::ceil(vtx.pos.x)));
    rc.bottom = std::max(rc.bottom, static_cast<LONG>(std::ceil(vtx.pos.y)));
  }

  return rc;
}

void FrameData::push_draw_cmd(DrawCmdType type, ImageHandle image_handle) noexcept
{
  if (auto res = _indices.size() - _draw_index_beg)
  {
    _draw_cmd_rect_idxs.emplace_back(_draw_cmds.size());
    
    auto cmd = DrawCmd{};
    cmd.type = type;
    cmd.ui.idx_beg      = _draw_index_beg;
    cmd.ui.idx_size     = res;
    cmd.ui.image_handle = image_handle;
    _draw_cmds.emplace_back(std::move(cmd));

    _draw_index_beg = _indices.size();
  }
}

void FrameData::push_draw_cmd_clear_rect(DrawCmdType type, std::optional<RECT> rect) noexcept
{
  auto cmd = DrawCmd{};
  cmd.type       = type;
  cmd.clear_rect = rect;
  _draw_cmds.emplace_back(std::move(cmd));
}

void FrameData::add_scissor_rect(RECT rect) noexcept
{
  push_draw_cmd(DrawCmdType::ui);

  for (auto idx : _draw_cmd_rect_idxs)
    _draw_cmds[idx].ui.scissor_rect = rect;
  _draw_cmd_rect_idxs.clear();
}

void FrameData::add_image(ImageHandle handle, vec2 left_top, vec2 right_bottom, uint8_t alpha) noexcept
{
  assert(!_using_discard_shapes);

  auto type = _use_discard ? DrawCmdType::discard_draw_tmp : DrawCmdType::ui;
  push_draw_cmd(type);

  auto col = Color{ 1, 1, 1, static_cast<float>(alpha) / 255 };

  auto [vertices, indices] = expand_beg(4, 6);
  vertices[0] = { left_top, {}, col };
  vertices[1] = { { right_bottom.x, left_top.y }, { 1, 0 }, col };
  vertices[2] = { right_bottom, { 1, 1 }, col };
  vertices[3] = { { left_top.x, right_bottom.y }, { 0, 1 }, col };
  indices[0]  = _vertex_beg + 0;
  indices[1]  = _vertex_beg + 1;
  indices[2]  = _vertex_beg + 2;
  indices[3]  = _vertex_beg + 0;
  indices[4]  = _vertex_beg + 2;
  indices[5]  = _vertex_beg + 3;
  expand_end();

  push_draw_cmd(type, handle);
}

void FrameData::discard_beg(std::function<void()> func) noexcept
{
  assert(!_use_discard);
  
  push_draw_cmd(DrawCmdType::ui);

  _using_discard_shapes = true;
  func();
  _using_discard_shapes = false;

  push_draw_cmd_clear_rect(DrawCmdType::clear_mask_image);
  push_draw_cmd_clear_rect(DrawCmdType::clear_tmp_image);
  push_draw_cmd(DrawCmdType::mask_write);

  _use_discard     = true;
  _discard_vtx_beg = _vertex_beg;
}

void FrameData::discard_end() noexcept
{
  assert(_use_discard && _vertices.size() > _discard_vtx_beg);

  _use_discard = false;

  auto rc = get_rect(_discard_vtx_beg, _vertices.size() - _discard_vtx_beg);
  push_draw_cmd(DrawCmdType::discard_draw_tmp);
  add_rect({ rc.left, rc.top }, { rc.right, rc.bottom });
  push_draw_cmd(DrawCmdType::composite_tmp);
}

void FrameData::union_beg() noexcept
{
  push_draw_cmd(DrawCmdType::ui);
  push_draw_cmd_clear_rect(DrawCmdType::clear_mask_image);
  push_draw_cmd_clear_rect(DrawCmdType::clear_tmp_image);
}

void FrameData::union_end() noexcept
{
  push_draw_cmd(DrawCmdType::mask_write);
}

}
