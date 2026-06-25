#include "frame_data.hpp"
#include "util.hpp"
#include "../ui_context.hpp"
#include "triangulator.hpp"

namespace tk::ui {

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

  auto col    = color.to_uint();
  vertices[0] = { left_top,                       {}, col };
  vertices[1] = { { right_bottom.x, left_top.y }, {}, col };
  vertices[2] = { right_bottom,                   {}, col };
  vertices[3] = { { left_top.x, right_bottom.y }, {}, col };
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

void FrameData::add_convex_poly_filled(Color color) noexcept
{
  auto pt_cnt = static_cast<uint>(_points.size());
  assert(pt_cnt > 2);

  auto aa_width = g_ui_ctx.window()->scale();
  auto aa_col   = Color{ color.r, color.g, color.b, 0.f }.to_uint();
  auto col      = color.to_uint();

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
    vertices[0] = { _points[i1] - dmp, {}, col    };
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
  auto col_trans = Color{ color.r, color.g, color.b, 0 }.to_uint();
  auto col       = color.to_uint();

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

    vtx[0] = { _points[i1] - dm, {}, col       };
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
  auto col_trans = Color{ color.r, color.g, color.b, 0 }.to_uint();
  auto col       = color.to_uint();

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
      vtx[0] = { tmp_points[i * 4],     {}, col_trans };
      vtx[1] = { tmp_points[i * 4 + 1], {}, col       };
      vtx[2] = { tmp_points[i * 4 + 2], {}, col       };
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
      vtx[0] = { _points[i],            {}, col       };
      vtx[1] = { tmp_points[i * 2 + 0], {}, col_trans };
      vtx[2] = { tmp_points[i * 2 + 1], {}, col_trans };
      vtx += 3;
    }
  }

  expand_end();
  _points.clear();
}

}
