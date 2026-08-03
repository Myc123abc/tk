#include "frame_data.hpp"
#include "util.hpp"

#include <numbers>

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

void FrameData::push_render_cmd(RenderCmdType type) noexcept
{
  auto idx_size = static_cast<uint>(_indices.size() - _draw_index_beg);
  if (!idx_size) return;

  if (type == RenderCmdType::ui)
    _render_cmd_rect_idxs.emplace_back(_render_cmds.size());

  auto& cmd = _render_cmds.emplace_back();
  cmd.type     = type;
  cmd.idx_beg  = _draw_index_beg;
  cmd.idx_size = idx_size;

  _draw_index_beg = static_cast<uint>(_indices.size());
}

void FrameData::push_render_cmd_clear_rect(RenderCmdType type, Rect rect) noexcept
{
  auto& cmd = _render_cmds.emplace_back();
  cmd.type = type;
  cmd.rect = rect;
}

void FrameData::build_render_cmd(DrawCmd const& cmd, uint& idx) noexcept
{
  using Type = DrawCmd::Type;

  auto const vtx_beg = _vertex_beg;
  auto should_transform_vertices = true;

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
      cmd.data.add_image.p0,
      cmd.data.add_image.p1,
      cmd.data.add_image.p2,
      cmd.data.add_image.p3,
      cmd.data.add_image.alpha,
      cmd.data.add_image.uv0,
      cmd.data.add_image.uv1,
      cmd.data.add_image.uv2,
      cmd.data.add_image.uv3
    );
    break;

  case Type::add_text:
    _add_text(cmd.data.add_text.handle, cmd.data.add_text.pos, cmd.data.add_text.size, cmd.data.add_text.inner_color, cmd.data.add_text.outer_color, cmd.data.add_text.outline_width);
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
    should_transform_vertices = false;
    _union_beg(idx);
    break;

  case Type::union_end:
    break;

  case Type::add_scissor_rect:
    should_transform_vertices = false;
    _add_scissor_rect(cmd.data.add_scissor_rect.rect);
    break;

  case Type::transform_beg:
    should_transform_vertices = false;
    _transform_beg(cmd.data.transform_beg.matrix);
    break;

  case Type::transform_end:
    should_transform_vertices = false;
    _transform_end();
    break;

  case Type::discard_beg:
    should_transform_vertices = false;
    _discard_beg(cmd.data.discard_beg.count, idx);
    break;

  case Type::discard_end:
    should_transform_vertices = false;
    _discard_end();
    break;
  }

  if (should_transform_vertices)
    transform_vertices(vtx_beg);
}

void FrameData::build_render_cmds() noexcept
{
  for (auto idx = 0u; idx < _draw_cmds.size(); ++idx)
    build_render_cmd(_draw_cmds[idx], idx);
  err_if(_build_mode.contains(BuildMode::uni), "union_end not be called");
  err_if(_build_mode.contains(BuildMode::discard), "discard_end not be called");
  err_if(!_transform_stack.empty(), "transform_end not be called");
}

}
