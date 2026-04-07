#include "frame_data.hpp"
#include "ui_context.hpp"

namespace {

auto sqrt(float x) noexcept
{
  return _mm_cvtss_f32(_mm_rsqrt_ss(_mm_set_ss(x)));
}

auto normalize(glm::vec2 p) noexcept
{
  auto d2 = glm::dot(p, p);
  if (d2 > .0f)
  {
    auto inv_len = sqrt(d2);
    return p * inv_len;
  }
  return glm::vec2{};
}

auto fix_normal(glm::vec2 p) noexcept
{
  auto d2 = glm::dot(p, p);
  if (d2 > 0.000001f)
  {
    auto inv_len2 = 1.f / d2;
    auto const max = 100.f;
    if (inv_len2 > max)
      inv_len2 = max;
    return p * inv_len2;
  }
  return glm::vec2{};
}

auto calc_circle_segment_count(int idx) noexcept
{
  return 0;
}

}

namespace tk::ui {

void FrameData::add_rect(glm::vec2 left_top, glm::vec2 right_bottom, Color color, float thickness) noexcept
{
  auto [vertices, indices] = expand_beg(4, 6);

  vertices[0] = { left_top, color };
  vertices[1] = { { right_bottom.x, left_top.y }, color };
  vertices[2] = { right_bottom, color };
  vertices[3] = { { left_top.x, right_bottom.y }, color };
  indices[0]  = _index + 0;
  indices[1]  = _index + 1;
  indices[2]  = _index + 2;
  indices[3]  = _index + 0;
  indices[4]  = _index + 2;
  indices[5]  = _index + 3;

  expand_end();
}

void FrameData::add_triangle(glm::vec2 p0, glm::vec2 p1, glm::vec2 p2, Color color, float thickness) noexcept
{
  set_points({ p0, p1, p2 });
  add_convex_poly_filled(color);
}

void FrameData::draw_circle(glm::vec2 center, float radius, Color color, float thickness) noexcept
{
  path_arc_to(center, radius);
}

void FrameData::add_convex_poly_filled(Color color) noexcept
{
  auto pt_cnt = _points.size();
  assert(pt_cnt > 2);

  auto aa_width = g_ui_ctx.get_scale();
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
    vertices[0] = { _points[i1] - dmp, color  };
    vertices[1] = { _points[i1] + dmp, aa_col };
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
}

auto FrameData::get_circle_segment_count(float radius) noexcept -> uint32_t
{
  int const idx = radius + 0.999999f;
  if (FrameData::_circle_segment_counts.contains(idx))
    return _circle_segment_counts[idx];
  _circle_segment_counts[idx] = calc_circle_segment_count(idx);
  return _circle_segment_counts[idx];
}

void FrameData::path_arc_to(glm::vec2 center, float radius) noexcept
{
  //auto step = 48 / get_circle_segment_count(radius);
}

void FrameData::add_scissor_rect(RECT rect) noexcept
{
  _draw_datas.emplace_back(rect, _draw_index_beg, _indices.size() - _draw_index_beg);
  _draw_index_beg = _indices.size();
}

}
