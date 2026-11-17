module tk.ui;

import :path;
import :ui.util;
import tk.base;
import tk.renderer;
import tk.string_id;
import tk.ui.context;

namespace tk::ui {

void path_begin(float2 p0) noexcept
{
  g_ctx.check_draw();
  detail::adjust_pos(p0);
  g_ctx.frame_data()->path_begin(p0);
}

void path_line_to(float2 p1) noexcept
{
  g_ctx.check_draw();
  detail::adjust_pos(p1);
  g_ctx.frame_data()->add_path_line_to(p1);
}

void path_arc_to(float2 center, float2 p1, bool ccw) noexcept
{
  g_ctx.check_draw();
  detail::adjust_pos(center, p1);
  g_ctx.frame_data()->add_path_arc_to(center, p1, ccw);
}

void path_quad_bezier_to(float2 p1, float2 p2) noexcept
{
  g_ctx.check_draw();
  detail::adjust_pos(p1, p2);
  g_ctx.frame_data()->add_path_quad_bezier_to(p1, p2);
}

void path_cubic_bezier_to(float2 p1, float2 p2, float2 p3) noexcept
{
  g_ctx.check_draw();
  detail::adjust_pos(p1, p2, p3);
  g_ctx.frame_data()->add_path_cubic_bezier_to(p1, p2, p3);
}

void path_end(bool close, Color color, float thickness) noexcept
{
  g_ctx.check_draw();
  detail::adjust_scale(thickness);
  g_ctx.frame_data()->path_end(close, color, thickness);
}

void union_beg() noexcept
{
  g_ctx.check_draw();
  g_ctx.frame_data()->union_beg();
}

void union_beg(StringID cache_id) noexcept
{
  g_ctx.check_draw();
  g_ctx.frame_data()->union_beg(g_ctx.get_id(cache_id));
}

void union_end(Color color, float thickness) noexcept
{
  g_ctx.check_draw();
  detail::adjust_scale(thickness);
  g_ctx.frame_data()->union_end(color, thickness);
}

}
