#include "frame_data.hpp"
#include "util.hpp"
#include "../ui_context.hpp"
#include "triangulator.hpp"
#include "../../renderer/resource/line_coverage_atlas.hpp"

namespace tk::ui {

namespace {

auto fix_rect_flags(Flag<CornerFlag> flags) noexcept -> Flag<CornerFlag>
{
  auto const mask = CornerFlag::all | CornerFlag::none;
  if ((flags & mask).empty())
    flags |= CornerFlag::all;
  return flags;
}

auto calc_arc_step_and_count(int segment_count, int sample_count) noexcept -> std::pair<int, int>
{
  auto step = std::clamp(
    sample_count / segment_count,
    1,
    sample_count / 4
  );
  auto step_count = (sample_count / 4) / step;
  if (step * step_count != sample_count / 4)
    step = (sample_count / 4) / step_count;
  return { step, step_count };
}

}

void FrameData::path_rect(float2 left_top, float2 right_bottom, float rounding, Flag<CornerFlag> flags, float tessellation_scale) noexcept
{
  if (rounding >= .5f)
  {
    flags = fix_rect_flags(flags);
    auto const round_top_bottom = flags.all(CornerFlag::top_left, CornerFlag::top_right)   || flags.all(CornerFlag::bottom_left, CornerFlag::bottom_right);
    auto const round_left_right = flags.all(CornerFlag::top_left, CornerFlag::bottom_left) || flags.all(CornerFlag::top_right,   CornerFlag::bottom_right);
    rounding = std::min(rounding, std::abs(right_bottom.x - left_top.x) * (round_top_bottom ? .5f : 1.f) - 1.f);
    rounding = std::min(rounding, std::abs(right_bottom.y - left_top.y) * (round_left_right ? .5f : 1.f) - 1.f);
  }

  if (rounding < .5f || flags.contains(CornerFlag::none))
  {
    _points.emplace_back(left_top);
    _points.emplace_back(right_bottom.x, left_top.y);
    _points.emplace_back(right_bottom);
    _points.emplace_back(left_top.x, right_bottom.y);
    return;
  }

  auto const pi = std::numbers::pi_v<float>;
  auto const rounding_top_left     = flags.contains(CornerFlag::top_left)     ? rounding : 0.f;
  auto const rounding_top_right    = flags.contains(CornerFlag::top_right)    ? rounding : 0.f;
  auto const rounding_bottom_right = flags.contains(CornerFlag::bottom_right) ? rounding : 0.f;
  auto const rounding_bottom_left  = flags.contains(CornerFlag::bottom_left)  ? rounding : 0.f;

  path_arc_to({ left_top.x     + rounding_top_left,     left_top.y     + rounding_top_left     }, rounding_top_left,     pi,        pi * 1.5f, tessellation_scale);
  path_arc_to({ right_bottom.x - rounding_top_right,    left_top.y     + rounding_top_right    }, rounding_top_right,    pi * 1.5f, pi * 2.f,  tessellation_scale);
  path_arc_to({ right_bottom.x - rounding_bottom_right, right_bottom.y - rounding_bottom_right }, rounding_bottom_right, 0,         pi * .5f,  tessellation_scale);
  path_arc_to({ left_top.x     + rounding_bottom_left,  right_bottom.y - rounding_bottom_left  }, rounding_bottom_left,  pi * .5f,  pi,        tessellation_scale);
}

void FrameData::add_rect(float2 left_top, float2 right_bottom, Color color, float thickness, float rounding, Flag<CornerFlag> flags) noexcept
{
  auto& cmd = _draw_cmds.emplace_back(DrawCmd::Type::add_rect);
  cmd.data.add_rect = { left_top, right_bottom, color, thickness, rounding, flags };
}

void FrameData::_add_rect(float2 left_top, float2 right_bottom, Color color, float thickness, float rounding, Flag<CornerFlag> flags) noexcept
{
  if (_using_discard_shapes) color.a = 1.f;
  else if (color.a == 0) return;

  auto const has_rounding = rounding >= .5f && !flags.contains(CornerFlag::none);

  if (thickness > 0)
  {
    assert(_points.empty());
    path_rect(left_top, right_bottom, rounding, flags);
    add_poly_line(color, thickness, true, true);
    return;
  }

  auto const is_pixel_aligned =
    std::trunc(left_top.x)     == left_top.x     &&
    std::trunc(left_top.y)     == left_top.y     &&
    std::trunc(right_bottom.x) == right_bottom.x &&
    std::trunc(right_bottom.y) == right_bottom.y;
  if (has_rounding || (!_using_discard_shapes && color.a < 1.f && !is_pixel_aligned))
  {
    assert(_points.empty());
    path_rect(left_top, right_bottom, rounding, flags);
    add_convex_poly_filled(color);
    return;
  }

  add_rect(left_top, right_bottom, color);
}

void FrameData::_add_transformed_rect(float2 left_top, float2 right_bottom, Color color, float thickness, float rounding, Flag<CornerFlag> flags, float scale) noexcept
{
  if (_using_discard_shapes) color.a = 1.f;
  else if (color.a == 0) return;

  assert(_points.empty());
  path_rect(left_top, right_bottom, rounding, flags, scale);
  for (auto& point : _points)
    point = transform_point(point);

  if (thickness > 0)
    add_poly_line(color, thickness * scale, true, true);
  else
    add_convex_poly_filled(color);
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
    add_poly_line(color, thickness, true, true);
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

  if (thickness <= 0)
  {
    auto const min_radius = g_ui_ctx.window()->scale() * std::numbers::sqrt2_v<float> * .5f;
    if (radius < min_radius)
    {
      color.a *= radius / min_radius;
      radius = min_radius;
    }
  }

  assert(_points.empty());
  path_arc_to(center, radius, 0, std::numbers::pi_v<float> * 2.f);
  if (!_points.empty())
    _points.pop_back();

  if (thickness > 0)
  {
    if (thickness >= radius)
      add_convex_poly_filled(color);
    else
      add_poly_line(color, thickness, true, true);
  }
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

  auto const aa_size         = g_ui_ctx.window()->scale();
  auto const can_pixel_align = _transform_stack.empty() || axis_aligned_uniform_transform_scale().has_value();
  if (!_using_discard_shapes && can_pixel_align && thickness <= aa_size)
  {
    auto const visual_thickness = std::max(thickness, aa_size);
    auto const align_center     = [visual_thickness](float value)
    {
      return std::round(value - visual_thickness * .5f) + visual_thickness * .5f;
    };
    if (p0.y == p1.y)
      p0.y = p1.y = align_center(p0.y);
    else if (p0.x == p1.x)
      p0.x = p1.x = align_center(p0.x);
  }

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
  path_bezier_quad_curve_to_casteljau(p0, p1, p2, curve_tessellation_max_error * curve_tessellation_max_error, 0);
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
  path_bezier_cubic_curve_to_casteljau(p0, p1, p2, p3, curve_tessellation_max_error * curve_tessellation_max_error, 0);
  add_poly_line(color, thickness, false);
}

void FrameData::add_convex_poly_filled(Color color) noexcept
{
  auto const pt_cnt = static_cast<uint>(_points.size());
  assert(pt_cnt > 2);

  auto const half_aa   = g_ui_ctx.window()->scale() * .5f;
  auto const aa_col    = Color{ color.r, color.g, color.b, 0.f }.to_uint();
  auto const col       = color.to_uint();
  auto const miter_max = Polyline_Miter_Limit * Polyline_Miter_Limit;

  _tmp_buf.resize(pt_cnt * 2);
  auto* normals    = _tmp_buf.data();
  auto* sq_lengths = normals + pt_cnt;
  auto  winding = 0.f;
  for (auto index = 0u; index < pt_cnt; ++index)
    winding += cross(_points[index], _points[(index + 1) % pt_cnt]);
  auto const normal_sign = winding < 0 ? -1.f : 1.f;
  for (uint i0 = pt_cnt - 1, i1 = 0; i1 < pt_cnt; i0 = i1++)
  {
    auto const delta = _points[i1] - _points[i0];
    auto const len_sq = length_sq(delta);
    auto const direction = len_sq > 0 ? delta / std::sqrt(len_sq) : float2{};
    normals[i0] = float2{ direction.y, -direction.x } * normal_sign;
    sq_lengths[i0].x = len_sq;
  }

  auto bevel_count = 0u;
  for (uint i0 = pt_cnt - 1, i1 = 0; i1 < pt_cnt; i0 = i1++)
  {
    auto const cos_theta = dot(normals[i0], normals[i1]);
    auto const clamped_cos = std::max(Polyline_Miter_Angle_Limit, cos_theta);
    auto const miter_scale = std::min(1000.f, 1.f / (1.f + clamped_cos));
    auto const miter = (normals[i0] + normals[i1]) * miter_scale;
    bevel_count += length_sq(miter) > miter_max;
  }

  auto const vtx_cnt = pt_cnt * 2 + bevel_count;
  auto const idx_cnt = (pt_cnt - 2) * 3 + pt_cnt * 6 + bevel_count * 3;
  auto [vertices, indices] = expand_beg(vtx_cnt, idx_cnt);

  auto const inner_idx = _vertex_beg;
  auto  next_outer_idx = _vertex_beg + pt_cnt;
  auto  previous_outer_idx = 0u;
  auto* inner_vertices = vertices;
  auto* outer_vertices = vertices + pt_cnt;
  auto* first_fringe_indices = indices;

  for (uint i0 = pt_cnt - 1, i1 = 0; i1 < pt_cnt; i0 = i1++)
  {
    auto const normal0 = normals[i0];
    auto const normal1 = normals[i1];
    auto const cos_theta = dot(normal0, normal1);
    auto const clamped_cos = std::max(Polyline_Miter_Angle_Limit, cos_theta);
    auto const miter_scale = std::min(1000.f, 1.f / (1.f + clamped_cos));
    auto miter = (normal0 + normal1) * miter_scale;
    auto const miter_distance_sq = length_sq(miter);
    auto const bevel = miter_distance_sq > miter_max;
    if (bevel)
    {
      auto const reference_half_sq = half_aa * half_aa;
      auto const limit_sq = std::max(sq_lengths[i0].x, sq_lengths[i1].x);
      auto const reference_miter_sq = miter_distance_sq * reference_half_sq;
      if (reference_miter_sq > limit_sq)
        miter *= std::sqrt(limit_sq / reference_miter_sq);
    }
    miter *= half_aa;

    inner_vertices[i1] = { _points[i1] - miter, {}, col };
    auto const previous_inner_idx = inner_idx + i0;
    auto const current_inner_idx  = inner_idx + i1;
    auto current_outer_idx = next_outer_idx;

    if (bevel)
    {
      *outer_vertices++ = { _points[i1] + normal0 * half_aa, {}, aa_col };
      *outer_vertices++ = { _points[i1] + normal1 * half_aa, {}, aa_col };
      next_outer_idx += 2;

      indices[0] = static_cast<uint16>(previous_outer_idx);
      indices[1] = static_cast<uint16>(current_outer_idx);
      indices[2] = static_cast<uint16>(current_inner_idx);
      indices[3] = static_cast<uint16>(previous_outer_idx);
      indices[4] = static_cast<uint16>(current_inner_idx);
      indices[5] = static_cast<uint16>(previous_inner_idx);
      indices[6] = static_cast<uint16>(current_outer_idx);
      indices[7] = static_cast<uint16>(current_outer_idx + 1);
      indices[8] = static_cast<uint16>(current_inner_idx);
      indices += 9;
      ++current_outer_idx;
    }
    else
    {
      *outer_vertices++ = { _points[i1] + miter, {}, aa_col };
      ++next_outer_idx;

      indices[0] = static_cast<uint16>(previous_outer_idx);
      indices[1] = static_cast<uint16>(current_outer_idx);
      indices[2] = static_cast<uint16>(current_inner_idx);
      indices[3] = static_cast<uint16>(previous_outer_idx);
      indices[4] = static_cast<uint16>(current_inner_idx);
      indices[5] = static_cast<uint16>(previous_inner_idx);
      indices += 6;
    }
    previous_outer_idx = current_outer_idx;
  }

  first_fringe_indices[0] = static_cast<uint16>(previous_outer_idx);
  first_fringe_indices[3] = static_cast<uint16>(previous_outer_idx);
  for (auto i = 2u; i < pt_cnt; ++i)
  {
    indices[0] = static_cast<uint16>(inner_idx);
    indices[1] = static_cast<uint16>(inner_idx + i - 1);
    indices[2] = static_cast<uint16>(inner_idx + i);
    indices += 3;
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

void FrameData::path_arc_to(float2 center, float radius, float min, float max, float tessellation_scale) noexcept
{
  if (radius < .5f)
  {
    _points.emplace_back(center);
    return;
  }

  if (tessellation_scale != 1.f)
  {
    auto const arc_len            = std::abs(max - min);
    auto const circle_segment_cnt = calc_circle_segment_count(radius * tessellation_scale);
    auto const arc_segment_cnt    = std::max(std::ceil(circle_segment_cnt * arc_len / (std::numbers::pi * 2.f)), 1.0);
    _path_arc_to(center, radius, min, max, static_cast<int>(arc_segment_cnt));
    return;
  }

  if (radius < arc_radius_cutoff)
  {
    auto const is_reverse = max < min;
    auto const [arc_step, arc_step_count] = calc_arc_step_and_count(static_cast<int>(get_circle_segment_count(radius)), arc_sample_max);
    auto const arc_segment_count = arc_step_count * 4;
    auto const min_sample_f = arc_segment_count * min / (std::numbers::pi * 2.f);
    auto const max_sample_f = arc_segment_count * max / (std::numbers::pi * 2.f);
    auto const min_sample = (is_reverse ? std::floor(min_sample_f) : std::ceil(min_sample_f)) * arc_step;
    auto const max_sample = (is_reverse ? std::ceil(max_sample_f) : std::floor(max_sample_f)) * arc_step;
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
  auto [step, _] = calc_arc_step_and_count(static_cast<int>(get_circle_segment_count(radius)), arc_sample_max);
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

void FrameData::_path_arc_to(float2 center, float radius, float min, float max, int segment_cnt) noexcept
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

void FrameData::add_poly_line(Color color, float thickness, bool is_closed, bool stroke_inside) noexcept
{
  auto const unique_end = std::unique(_points.begin(), _points.end());
  _points.erase(unique_end, _points.end());
  if (is_closed && _points.size() > 2 && _points.front() == _points.back())
    _points.pop_back();
  if (_points.size() < 2)
  {
    _points.clear();
    return;
  }

  is_closed &= _points.size() > 2;
  stroke_inside &= is_closed;
  auto const aa_size = g_ui_ctx.window()->scale();
  if (thickness < aa_size / 255.f)
  {
    _points.clear();
    return;
  }
  if (thickness < aa_size)
  {
    color.a *= thickness / aa_size;
    thickness = aa_size;
  }
  if (color.a == 0)
  {
    _points.clear();
    return;
  }
  auto const col        = color.to_uint();
  auto const col_trans  = Color{ color.r, color.g, color.b, 0 }.to_uint();
  auto const texture_transform_supported = _transform_stack.empty() || uniform_transform_scale().has_value();
  auto const texture = _using_discard_shapes || !texture_transform_supported
    ? std::optional<renderer::LineCoverageAtlas::Selection>{}
    : renderer::g_line_coverage_atlas.select(thickness, aa_size);
  auto const fringe_mode = texture
    ? PolylineTessellator::FringeMode::texture_coverage
    : PolylineTessellator::FringeMode::geometry;
  auto const mesh = _polyline_tessellator.build(
    _points,
    thickness,
    texture ? texture->fringe : aa_size,
    is_closed,
    stroke_inside,
    fringe_mode
  );
  auto const vtx_cnt    = static_cast<uint>(mesh.vertices.size());
  auto const idx_cnt    = static_cast<uint>(mesh.indices.size());
  auto [vertices, indices] = expand_beg(vtx_cnt, idx_cnt);

  if (texture)
  {
    auto const packed = renderer::vtx_pack(renderer::VtxType::line, renderer::g_img_mgr[texture->image].srv().index());
    for (auto const& vertex : mesh.vertices)
    {
      auto const uv = float2
      {
        std::lerp(texture->uv0.x, texture->uv1.x, vertex.coverage),
        texture->uv0.y,
      };
      *vertices++ = { vertex.pos, uv, col, packed };
    }
    _sampled_images.emplace(texture->image);
  }
  else
    for (auto const& vertex : mesh.vertices)
      *vertices++ = { vertex.pos, {}, vertex.opaque ? col : col_trans };
  for (auto index : mesh.indices)
    *indices++ = static_cast<uint16>(_vertex_beg + index);

  expand_end();
  _points.clear();
}

}
