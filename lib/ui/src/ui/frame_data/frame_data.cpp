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

  if (type == RenderCmdType::ui || type == RenderCmdType::discard_composite)
    _render_cmd_rect_idxs.emplace_back(_render_cmds.size());

  auto& cmd = _render_cmds.emplace_back();
  cmd.type     = type;
  cmd.idx_beg  = _draw_index_beg;
  cmd.idx_size = idx_size;

  _draw_index_beg = static_cast<uint>(_indices.size());
}

void FrameData::push_render_cmd_clear_rect(RenderCmdType type, Rect rect) noexcept
{
  _render_cmd_clip_rect_idxs.emplace_back(_render_cmds.size());
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
  {
    auto left_top     = cmd.data.add_rect.left_top;
    auto right_bottom = cmd.data.add_rect.right_bottom;
    auto thickness    = cmd.data.add_rect.thickness;
    auto rounding     = cmd.data.add_rect.rounding;
    auto const bounds = std::array
    {
      left_top,
      float2{ right_bottom.x, left_top.y },
      right_bottom,
      float2{ left_top.x, right_bottom.y },
    };
    if (should_cull(bounds, thickness))
      break;
    if (auto scale = axis_aligned_uniform_transform_scale())
    {
      left_top     = transform_point(left_top);
      right_bottom = transform_point(right_bottom);
      thickness   *= scale.value();
      rounding    *= scale.value();
      should_transform_vertices = false;
      _add_rect(left_top, right_bottom, cmd.data.add_rect.color, thickness, rounding, cmd.data.add_rect.flags);
    }
    else if (auto scale = uniform_transform_scale())
    {
      should_transform_vertices = false;
      _add_transformed_rect(left_top, right_bottom, cmd.data.add_rect.color, thickness, rounding, cmd.data.add_rect.flags, scale.value());
    }
    else
      _add_rect(left_top, right_bottom, cmd.data.add_rect.color, thickness, rounding, cmd.data.add_rect.flags);
    break;
  }

  case Type::add_triangle:
  {
    auto p0        = cmd.data.add_triangle.p0;
    auto p1        = cmd.data.add_triangle.p1;
    auto p2        = cmd.data.add_triangle.p2;
    auto thickness = cmd.data.add_triangle.thickness;
    if (auto const bounds = std::array{ p0, p1, p2 }; should_cull(bounds, thickness))
      break;
    if (auto scale = uniform_transform_scale())
    {
      p0 = transform_point(p0);
      p1 = transform_point(p1);
      p2 = transform_point(p2);
      thickness *= scale.value();
      should_transform_vertices = false;
    }
    _add_triangle(p0, p1, p2, cmd.data.add_triangle.color, thickness);
    break;
  }

  case Type::add_circle:
  {
    auto center    = cmd.data.add_circle.center;
    auto radius    = cmd.data.add_circle.radius;
    auto thickness = cmd.data.add_circle.thickness;
    auto const bounds = std::array
    {
      center + float2{ -radius, -radius },
      center + float2{  radius, -radius },
      center + float2{  radius,  radius },
      center + float2{ -radius,  radius },
    };
    if (should_cull(bounds, thickness))
      break;
    if (auto scale = uniform_transform_scale())
    {
      center = transform_point(center);
      radius *= scale.value();
      thickness *= scale.value();
      should_transform_vertices = false;
    }
    if (radius > 0)
      _add_circle(center, radius, cmd.data.add_circle.color, thickness);
    break;
  }

  case Type::add_line:
  {
    auto p0        = cmd.data.add_line.p0;
    auto p1        = cmd.data.add_line.p1;
    auto thickness = cmd.data.add_line.thickness;
    if (auto const bounds = std::array{ p0, p1 }; should_cull(bounds, thickness))
      break;
    if (auto scale = uniform_transform_scale())
    {
      p0 = transform_point(p0);
      p1 = transform_point(p1);
      thickness *= scale.value();
      should_transform_vertices = false;
    }
    _add_line(p0, p1, cmd.data.add_line.color, thickness);
    break;
  }

  case Type::add_arc:
  {
    auto center    = cmd.data.add_arc.center;
    auto p0        = cmd.data.add_arc.p0;
    auto p1        = cmd.data.add_arc.p1;
    auto thickness = cmd.data.add_arc.thickness;
    auto const radius = length(p0 - center);
    auto const bounds = std::array
    {
      center + float2{ -radius, -radius },
      center + float2{  radius, -radius },
      center + float2{  radius,  radius },
      center + float2{ -radius,  radius },
    };
    if (should_cull(bounds, thickness))
      break;
    if (auto scale = uniform_transform_scale())
    {
      center = transform_point(center);
      p0 = transform_point(p0);
      p1 = transform_point(p1);
      thickness *= scale.value();
      should_transform_vertices = false;
    }
    _add_arc(center, p0, p1, cmd.data.add_arc.color, thickness);
    break;
  }

  case Type::add_quad_bezier:
  {
    auto p0        = cmd.data.add_quad_bezier.p0;
    auto p1        = cmd.data.add_quad_bezier.p1;
    auto p2        = cmd.data.add_quad_bezier.p2;
    auto thickness = cmd.data.add_quad_bezier.thickness;
    if (auto const bounds = std::array{ p0, p1, p2 }; should_cull(bounds, thickness))
      break;
    if (auto scale = uniform_transform_scale())
    {
      p0 = transform_point(p0);
      p1 = transform_point(p1);
      p2 = transform_point(p2);
      thickness *= scale.value();
      should_transform_vertices = false;
    }
    _add_quad_bezier(p0, p1, p2, cmd.data.add_quad_bezier.color, thickness);
    break;
  }

  case Type::add_cubic_bezier:
  {
    auto p0        = cmd.data.add_cubic_bezier.p0;
    auto p1        = cmd.data.add_cubic_bezier.p1;
    auto p2        = cmd.data.add_cubic_bezier.p2;
    auto p3        = cmd.data.add_cubic_bezier.p3;
    auto thickness = cmd.data.add_cubic_bezier.thickness;
    if (auto const bounds = std::array{ p0, p1, p2, p3 }; should_cull(bounds, thickness))
      break;
    if (auto scale = uniform_transform_scale())
    {
      p0 = transform_point(p0);
      p1 = transform_point(p1);
      p2 = transform_point(p2);
      p3 = transform_point(p3);
      thickness *= scale.value();
      should_transform_vertices = false;
    }
    _add_cubic_bezier(p0, p1, p2, p3, cmd.data.add_cubic_bezier.color, thickness);
    break;
  }

  case Type::add_image:
  {
    auto const bounds = std::array
    {
      cmd.data.add_image.p0,
      cmd.data.add_image.p1,
      cmd.data.add_image.p2,
      cmd.data.add_image.p3,
    };
    if (should_cull(bounds))
      break;
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
  }

  case Type::add_image_rounded:
  {
    auto const left_top     = cmd.data.add_image_rounded.left_top;
    auto const right_bottom = cmd.data.add_image_rounded.right_bottom;
    auto const bounds       = std::array
    {
      left_top,
      float2{ right_bottom.x, left_top.y },
      right_bottom,
      float2{ left_top.x, right_bottom.y },
    };
    if (should_cull(bounds))
      break;
    _add_image_rounded(
      cmd.data.add_image_rounded.handle,
      cmd.data.add_image_rounded.left_top,
      cmd.data.add_image_rounded.right_bottom,
      cmd.data.add_image_rounded.alpha,
      cmd.data.add_image_rounded.uv0,
      cmd.data.add_image_rounded.uv1,
      cmd.data.add_image_rounded.uv2,
      cmd.data.add_image_rounded.uv3,
      cmd.data.add_image_rounded.rounding,
      cmd.data.add_image_rounded.flags
    );
    break;
  }

  case Type::add_text:
  {
    auto const& result = g_text_engine.get_parse_result(cmd.data.add_text.handle);
    auto const  pos    = cmd.data.add_text.pos;
    auto const  extent = result.extent * cmd.data.add_text.scale;
    auto const  bounds = std::array
    {
      pos,
      pos + float2{ extent.x, 0 },
      pos + extent,
      pos + float2{ 0, extent.y },
    };
    auto const glyph_padding = (result.is_vertical ? extent.x : extent.y) + 2.f;
    if (should_cull(bounds, glyph_padding))
      break;
    _add_text(cmd.data.add_text.handle, cmd.data.add_text.pos, cmd.data.add_text.scale, cmd.data.add_text.inner_color, cmd.data.add_text.outer_color, cmd.data.add_text.outline_width);
    break;
  }

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
  {
    auto thickness = cmd.data.path_end.thickness;
    if (should_cull(_points, thickness))
    {
      _points.clear();
      break;
    }
    if (auto scale = uniform_transform_scale())
    {
      for (auto& point : _points)
        point = transform_point(point);
      if (thickness > 0)
        thickness *= scale.value();
      should_transform_vertices = false;
    }
    _path_end(cmd.data.path_end.close, cmd.data.path_end.color, thickness);
    break;
  }

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

  case Type::cull_rect_beg:
    should_transform_vertices = false;
    _cull_rect_beg(cmd.data.cull_rect_beg.rect);
    break;

  case Type::cull_rect_end:
    should_transform_vertices = false;
    _cull_rect_end();
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
  assert(_cull_rect_stack.empty());
  if (_base_cull_rect)
    _cull_rect_stack.emplace_back(_base_cull_rect.value());
  auto const base_cull_depth = _cull_rect_stack.size();

  for (auto idx = 0u; idx < _draw_cmds.size(); ++idx)
    build_render_cmd(_draw_cmds[idx], idx);
  err_if(_build_mode.contains(BuildMode::uni), "union_end not be called");
  err_if(_build_mode.contains(BuildMode::discard), "discard_end not be called");
  err_if(!_transform_stack.empty(), "transform_end not be called");
  err_if(_cull_rect_stack.size() != base_cull_depth, "cull_rect_end not be called");
  _cull_rect_stack.clear();
}

}
