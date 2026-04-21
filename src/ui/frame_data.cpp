#include "frame_data.hpp"
#include "ui_context.hpp"

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

auto calc_circle_segment_count(float radius) noexcept
{
  auto constexpr tessellation_max_error = 0.3f;
  auto constexpr segment_min            = 4;
  auto constexpr segment_max            = 512;
  return std::clamp(
    round_up_to_even(std::ceil(
      std::numbers::pi /
      std::acos(1 - std::min(tessellation_max_error, radius) / radius)
    )),
    segment_min, segment_max);
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
}

void FrameData::add_rect(vec2 left_top, vec2 right_bottom, Color color, float thickness) noexcept
{
  if (color.a == 0) return;

  auto [vertices, indices] = expand_beg(4, 6);

  vertices[0] = { left_top, {}, color };
  vertices[1] = { { right_bottom.x, left_top.y }, {}, color };
  vertices[2] = { right_bottom, {}, color };
  vertices[3] = { { left_top.x, right_bottom.y }, {}, color };
  indices[0]  = _index + 0;
  indices[1]  = _index + 1;
  indices[2]  = _index + 2;
  indices[3]  = _index + 0;
  indices[4]  = _index + 2;
  indices[5]  = _index + 3;

  expand_end();
}

void FrameData::add_triangle(vec2 p0, vec2 p1, vec2 p2, Color color, float thickness) noexcept
{
  if (color.a == 0) return;
  set_points({ p0, p1, p2 });
  add_convex_poly_filled(color);
}

void FrameData::draw_circle(vec2 center, float radius, Color color, float thickness) noexcept
{
  if (color.a == 0) return;
  path_arc_to(center, radius);
  _points.pop_back();
  add_convex_poly_filled(color);
}

void FrameData::add_convex_poly_filled(Color color) noexcept
{
  auto pt_cnt = _points.size();
  assert(pt_cnt > 2);

  auto aa_width = g_ui_ctx.window()->scale();
  auto aa_col   = Color{ color.r, color.g, color.b, 0.f };

  auto [vertices, indices] = expand_beg(pt_cnt * 2, (pt_cnt - 2) * 3 + pt_cnt * 6);

  auto inner_idx = _index;
  auto outer_idx = _index + 1;
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

auto FrameData::get_circle_segment_count(float radius) noexcept -> uint32_t
{
  int const idx = radius + 0.999999f;
  if (idx >= 0 && idx < _circle_segment_counts.size())
    return _circle_segment_counts[idx];
  return calc_circle_segment_count(radius);
}

void FrameData::path_arc_to(vec2 center, float radius) noexcept
{
  if (radius < .5f)
  {
    set_points({ center });
    return;
  }

  auto step = std::clamp(arc_sample_max / static_cast<int>(get_circle_segment_count(radius)),
    1, arc_table_size / 4);
  
  auto constexpr max_sample   = arc_sample_max;
  auto constexpr min_sample   = 0;
  auto constexpr sample_range = max_sample - min_sample;
  auto const     next_step    = step;

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

  auto sample_index = min_sample;
  if (sample_index < 0 || sample_index >= arc_sample_max)
  {
    sample_index %= arc_sample_max;
    if (sample_index < 0)
      sample_index += arc_sample_max;
  }

  if (max_sample >= min_sample)
  {
    for (auto a = min_sample; a <= max_sample; a += step, sample_index += step, step = next_step)
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
    for (auto a = min_sample; a >= max_sample; a -= step, sample_index -= step, step = next_step)
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
    auto normalized_max_sample = max_sample % arc_sample_max;
    if (normalized_max_sample < 0)
      normalized_max_sample += arc_sample_max;

    auto const s = _arc_vertices[normalized_max_sample];
    out_ptr->x = center.x + s.x * radius;
    out_ptr->y = center.y + s.y * radius;
    ++out_ptr;
  }

  assert(_points.data() + _points.size() == out_ptr);
}

void FrameData::add_draw_call(DrawDataType type, ImageHandle image_handle, uint8_t image_alpha) noexcept
{
  _draw_data_rect_idxs.emplace_back(_draw_datas.size());
  _draw_datas.emplace_back(type, _draw_index_beg, _indices.size() - _draw_index_beg, image_handle, image_alpha);
  _draw_index_beg = _indices.size();
}

void FrameData::add_scissor_rect(RECT rect) noexcept
{
  add_draw_call(DrawDataType::shape);

  for (auto idx : _draw_data_rect_idxs)
    _draw_datas[idx].scissor_rect = rect;
  _draw_data_rect_idxs.clear();
}

void FrameData::add_image(ImageHandle handle, vec2 left_top, vec2 right_bottom, uint8_t alpha) noexcept
{
  // push exist shape draw call
  if (_draw_index_beg != _indices.size())
    add_draw_call(DrawDataType::shape);

  auto [vertices, indices] = expand_beg(4, 6);
  vertices[0] = { left_top, {}, {} };
  vertices[1] = { { right_bottom.x, left_top.y }, { 1, 0 }, {} };
  vertices[2] = { right_bottom, { 1, 1 }, {} };
  vertices[3] = { { left_top.x, right_bottom.y }, { 0, 1 }, {} };
  indices[0]  = _index + 0;
  indices[1]  = _index + 1;
  indices[2]  = _index + 2;
  indices[3]  = _index + 0;
  indices[4]  = _index + 2;
  indices[5]  = _index + 3;
  expand_end();

  add_draw_call(DrawDataType::image, handle, alpha);
}

}
