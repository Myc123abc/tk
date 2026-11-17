module;

#include <utility>

module tk.ui;

import :geometry;
import :ui.util;
import tk.base;
import tk.flag;
import tk.renderer;
import tk.ui.context;

namespace tk::ui {

void rectangle(float2 left_top, float2 right_bottom, Color color, float thickness, float rounding, Flag<CornerFlag> flags) noexcept
{
  g_ctx.check_draw();
  if (left_top.x >= right_bottom.x || left_top.y >= right_bottom.y) return;
  detail::adjust_pos(left_top, right_bottom);
  detail::adjust_scale(thickness, rounding);
  g_ctx.frame_data()->add_rect(left_top, right_bottom, color, thickness, rounding, flags);
}

void triangle(float2 p0, float2 p1, float2 p2, Color color, float thickness) noexcept
{
  g_ctx.check_draw();
  if ((p0.x == p1.x && p1.x == p2.x) || (p0.y == p1.y && p1.y == p2.y)) return;
  detail::adjust_pos(p0, p1, p2);
  detail::adjust_scale(thickness);
  g_ctx.frame_data()->add_triangle(p0, p1, p2, color, thickness);
}

void circle(float2 center, float radius, Color color, float thickness) noexcept
{
  g_ctx.check_draw();
  if (radius <= 0) return;
  detail::adjust_pos(center);
  detail::adjust_scale(radius, thickness);
  g_ctx.frame_data()->add_circle(center, radius, color, thickness);
}

void line(float2 p0, float2 p1, Color color, float thickness) noexcept
{
  g_ctx.check_draw();
  if (p0 == p1 || thickness <= 0) return;
  detail::adjust_pos(p0, p1);
  detail::adjust_scale(thickness);
  g_ctx.frame_data()->add_line(p0, p1, color, thickness);
}

void arc(float2 center, float2 p0, float2 p1, bool ccw, Color color, float thickness) noexcept
{
  g_ctx.check_draw();
  if (thickness <= 0) return;
  detail::adjust_pos(center, p0, p1);
  detail::adjust_scale(thickness);
  if (ccw) std::swap(p0, p1);
  g_ctx.frame_data()->add_arc(center, p0, p1, color, thickness);
}

void quad_bezier(float2 p0, float2 p1, float2 p2, Color color, float thickness) noexcept
{
  g_ctx.check_draw();
  if (thickness <= 0) return;
  detail::adjust_pos(p0, p1, p2);
  detail::adjust_scale(thickness);
  g_ctx.frame_data()->add_quad_bezier(p0, p1, p2, color, thickness);
}

void cubic_bezier(float2 p0, float2 p1, float2 p2, float2 p3, Color color, float thickness) noexcept
{
  g_ctx.check_draw();
  if (thickness <= 0) return;
  detail::adjust_pos(p0, p1, p2, p3);
  detail::adjust_scale(thickness);
  g_ctx.frame_data()->add_cubic_bezier(p0, p1, p2, p3, color, thickness);
}

}
