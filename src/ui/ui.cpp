#include "ui/ui.hpp"
#include "ui_context.hpp"
#include "text_engine/text_engine.hpp"
#include "../renderer/window/window_manager.hpp"

#include <stb_image.h>

using namespace tk::renderer;
using namespace tk::ui;

namespace {

auto intersect_rect(RECT lhs, RECT rhs) noexcept -> std::optional<RECT>
{
  auto res = RECT{};
  if (IntersectRect(&res, &lhs, &rhs))
    return res;
  return {};
}

template <typename... Args>
void adjust_pos(Args&... args) noexcept
{
  auto offset = g_ui_ctx.get_render_pos();
  ((args += offset), ...);
  auto wnd = g_ui_ctx.window();
  auto scale = wnd->scale();
  ((args *= scale), ...);
}

template <typename... Args>
void adjust_scale(Args&... args) noexcept
{
  auto scale = g_ui_ctx.window()->scale();
  ((args *= scale), ...);
}

void adjust_transform(Matrix& transform) noexcept
{
  auto offset = g_ui_ctx.get_render_pos();
  auto scale  = g_ui_ctx.window()->scale();
  auto inv    = 1.f / scale;

  auto to_window   = Matrix{ scale, 0, 0, scale, offset.x * scale, offset.y * scale };
  auto from_window = Matrix{ inv,   0, 0, inv,   -offset.x,         -offset.y         };
  transform = from_window * transform * to_window;
}

}

namespace tk::ui {

////////////////////////////////////////////////////////////////////////////////
///                              Misc
////////////////////////////////////////////////////////////////////////////////

auto lerp(Color x, Color y, float v) noexcept -> float4
{
  return
  {
    std::lerp(x.r, y.r, v),
    std::lerp(x.g, y.g, v),
    std::lerp(x.b, y.b, v),
    std::lerp(x.a, y.a, v)
  };
}

auto lerp(float2 x, float2 y, float v) noexcept -> float2
{
  return { std::lerp(x.x, y.x, v), std::lerp(x.y, y.y, v) };
}

auto delta_time() noexcept -> double
{
  return g_ui_ctx.delta_time();
}

auto image_extent(std::string_view path) noexcept -> float2
{
  return g_img_mgr.extent(path);
}

auto image(std::string_view path, float2 left_top, float2 right_bottom, uint8 alpha, std::optional<ImageConfig> cfg) noexcept -> std::expected<void, ImageLoadErrorType>
{
  if (!alpha || left_top.x == right_bottom.x || left_top.y == right_bottom.y) return {};
  adjust_pos(left_top, right_bottom);
  if (cfg && cfg->rounding_cfg) adjust_scale(cfg->rounding_cfg->radius);
  return g_ui_ctx.image(path, left_top, right_bottom, alpha, cfg);
}

auto load_image(std::string_view path) noexcept -> std::expected<void, ImageLoadErrorType>
{
  return g_img_mgr.try_load(path, {}, {}).transform([](auto&&) {});
}

auto load_font(std::string_view path) noexcept -> std::expected<FontInfo, FontLoadErrorType>
{
  return g_text_engine.load_font(path);
}

auto text(std::string_view text, float2 pos, float size, Color color, TextConfig cfg) noexcept -> TextResult
{
  if (text.empty()) return {};
  adjust_pos(pos); adjust_scale(size);
  return g_ui_ctx.text(text, pos, size, color, cfg);
}

auto text(std::string_view text, float size, TextConfig cfg) noexcept -> TextResult
{
  if (text.empty()) return {};
  adjust_scale(size);
  return g_ui_ctx.text(text, {}, size, {}, cfg);
}

auto ping_pong(std::string_view name, bool b, double forward_dur, double reverse_dur, Tween::Ease ease) noexcept -> double
{
  return g_ui_ctx.ping_pong(b, g_ui_ctx.generic_id(name), forward_dur, reverse_dur, ease);
}

void reset_tween(std::string_view name) noexcept
{
  g_ui_ctx.reset_tween(g_ui_ctx.get_id(name));
}

auto get_cursor_pos() noexcept -> float2
{
  return renderer::get_cursor_pos();
}

auto get_cursor_pos_on_window() noexcept -> float2
{
  g_ui_ctx.check_draw();
  return get_cursor_pos() - g_ui_ctx.window()->pos();
}

////////////////////////////////////////////////////////////////////////////////
///                             Window
////////////////////////////////////////////////////////////////////////////////

void begin(std::string_view name, int x, int y, uint width, uint height, bool* is_closed, WindowConfig const& cfg) noexcept
{
	g_ui_ctx.begin(name, x, y, width, height, is_closed, cfg);
}

void end() noexcept
{
	g_ui_ctx.end();
}

void add_move_invalid_area(float2 left_top, float2 right_bottom) noexcept
{
  g_ui_ctx.check_draw();
  if (g_ui_ctx.is_use_title_bar_now() && !g_ui_ctx.draw_title_bar)
  {
    left_top.y     += Title_Bar_Height;
    right_bottom.y += Title_Bar_Height;
  }

  auto scale = g_ui_ctx.window()->scale();
  left_top     *= scale;
  right_bottom *= scale;

  auto wnd = g_ui_ctx.window();
  if (auto rect = intersect_rect({ static_cast<LONG>(left_top.x), static_cast<LONG>(left_top.y),
                                   static_cast<LONG>(right_bottom.x), static_cast<LONG>(right_bottom.y) },
                                 { 0, 0, static_cast<LONG>(wnd->width()), static_cast<LONG>(wnd->height()) }))
    g_ui_ctx.window()->add_move_invalid_area(rect.value());
}

auto window_extent() noexcept -> float2
{
  g_ui_ctx.check_draw();
  return float2{ g_ui_ctx.window()->width(), g_ui_ctx.window()->height() } / g_ui_ctx.window()->scale();
}

auto window_drawable_extent() noexcept -> float2
{
  auto extent = window_extent();
  if (g_ui_ctx.is_use_title_bar_now())
    extent.y -= Title_Bar_Height;
  return extent;
}

auto is_fullscreen_window() noexcept -> bool
{
  g_ui_ctx.check_draw();
  return g_ui_ctx.window()->is_fullscreen();
}

void fullscreen_window() noexcept
{
  g_ui_ctx.check_draw();
  if (g_ui_ctx.window()->cfg().no_resize) return;
  g_ui_ctx.fullscreen_window();
}

void restore_fullscreen_window() noexcept
{
  g_ui_ctx.check_draw();
  if (g_ui_ctx.window()->cfg().no_resize) return;
  g_ui_ctx.restore_fullscreen_window();
}

void transform_beg(Matrix const& transform) noexcept
{
  g_ui_ctx.check_draw();
  auto adjusted = transform;
  adjust_transform(adjusted);
  g_ui_ctx.frame_data()->transform_beg(adjusted);
}

void transform_beg(Transform const& transform) noexcept
{
  transform_beg(transform.matrix());
}

void transform_end() noexcept
{
  g_ui_ctx.check_draw();
  g_ui_ctx.frame_data()->transform_end();
}


////////////////////////////////////////////////////////////////////////////////
///                            Shape Operator
////////////////////////////////////////////////////////////////////////////////

void discard_beg(std::function<void()> func) noexcept
{
  g_ui_ctx.frame_data()->discard_beg(func);
}

void discard_end() noexcept
{
  g_ui_ctx.frame_data()->discard_end();
}

////////////////////////////////////////////////////////////////////////////////
///                            Geometry
////////////////////////////////////////////////////////////////////////////////

void rectangle(float2 left_top, float2 right_bottom, Color color, float thickness, float rounding, Flag<CornerFlag> flags) noexcept
{
	g_ui_ctx.check_draw();

  if (left_top.x == right_bottom.x || left_top.y == right_bottom.y) return;

  adjust_pos(left_top, right_bottom); adjust_scale(thickness, rounding);
  g_ui_ctx.frame_data()->add_rect(left_top, right_bottom, color, thickness, rounding, flags);
}

void triangle(float2 p0, float2 p1, float2 p2, Color color, float thickness) noexcept
{
	g_ui_ctx.check_draw();

  if (p0.x == p1.x == p2.x || p0.y == p1.y == p2.y) return;

  adjust_pos(p0, p1, p2); adjust_scale(thickness);
	g_ui_ctx.frame_data()->add_triangle(p0, p1, p2, color, thickness);
}

void circle(float2 center, float radius, Color color, float thickness) noexcept
{
	g_ui_ctx.check_draw();

  if (radius <= 0) return;

  adjust_pos(center); adjust_scale(radius, thickness);
  g_ui_ctx.frame_data()->add_circle(center, radius, color, thickness);
}

void line(float2 p0, float2 p1, Color color, float thickness) noexcept
{
	g_ui_ctx.check_draw();
  if (p0 == p1) return;
  if (thickness < 1) thickness = 1;
  adjust_pos(p0, p1); adjust_scale(thickness);
  p0 = floor(p0) + .5f;
  p1 = floor(p1) + .5f;
  g_ui_ctx.frame_data()->add_line(p0, p1, color, thickness);
}

void arc(float2 center, float2 p0, float2 p1, bool ccw, Color color, float thickness) noexcept
{
  g_ui_ctx.check_draw();
  if (thickness < 1) thickness = 1;
  adjust_pos(center, p0, p1); adjust_scale(thickness);
  center = floor(center) + .5f;
  p0     = floor(p0)     + .5f;
  p1     = floor(p1)     + .5f;
  if (ccw) std::swap(p0, p1);
  g_ui_ctx.frame_data()->add_arc(center, p0, p1, color, thickness);
}

void quad_bezier(float2 p0, float2 p1, float2 p2, Color color, float thickness) noexcept
{
	g_ui_ctx.check_draw();
  if (thickness < 1) thickness = 1;
  adjust_pos(p0, p1, p2); adjust_scale(thickness);
  g_ui_ctx.frame_data()->add_quad_bezier(p0, p1, p2, color, thickness);
}

void cubic_bezier(float2 p0, float2 p1, float2 p2, float2 p3, Color color, float thickness) noexcept
{
	g_ui_ctx.check_draw();
  if (thickness < 1) thickness = 1;
  adjust_pos(p0, p1, p2, p3); adjust_scale(thickness);
  g_ui_ctx.frame_data()->add_cubic_bezier(p0, p1, p2, p3, color, thickness);
}

////////////////////////////////////////////////////////////////////////////////
///                               Path
////////////////////////////////////////////////////////////////////////////////

void path_begin(float2 p0) noexcept
{
  g_ui_ctx.check_draw();
  adjust_pos(p0);
  g_ui_ctx.frame_data()->path_begin(p0);
}

void path_line_to(float2 p1) noexcept
{
	g_ui_ctx.check_draw();
  adjust_pos(p1);
  g_ui_ctx.frame_data()->add_path_line_to(p1);
}

void path_arc_to(float2 center, float2 p1, bool ccw) noexcept
{
  g_ui_ctx.check_draw();
  adjust_pos(center, p1);
  g_ui_ctx.frame_data()->add_path_arc_to(center, p1, ccw);
}

void path_quad_bezier_to(float2 p1, float2 p2) noexcept
{
  g_ui_ctx.check_draw();
  adjust_pos(p1, p2);
  g_ui_ctx.frame_data()->add_path_quad_bezier_to(p1, p2);
}

void path_cubic_bezier_to(float2 p1, float2 p2, float2 p3) noexcept
{
	g_ui_ctx.check_draw();
  adjust_pos(p1, p2, p3);
  g_ui_ctx.frame_data()->add_path_cubic_bezier_to(p1, p2, p3);
}

void path_end(bool close, Color color, float thickness) noexcept
{
	g_ui_ctx.check_draw();
  adjust_scale(thickness);
  g_ui_ctx.frame_data()->path_end(close, color, thickness);
}

void union_beg() noexcept
{
  g_ui_ctx.check_draw();
  g_ui_ctx.frame_data()->union_beg();
}

void union_end(Color color, float thickness) noexcept
{
  g_ui_ctx.check_draw();
  adjust_scale(thickness);
  g_ui_ctx.frame_data()->union_end(color, thickness);
}

////////////////////////////////////////////////////////////////////////////////
///                               Key
////////////////////////////////////////////////////////////////////////////////

auto GetKeyResult::is_uppercase() const noexcept -> bool
{
  if (is_caps_locked()) return !get_key(Key::Shift).has_down();
  return get_key(Key::Shift).has_down();
}

auto get_key(Key key) noexcept -> GetKeyResult
{
  g_ui_ctx.check_draw();
  if (g_ui_ctx.window()->is_active())
    return { g_ui_ctx.get_key(key) };
  return {};
}

}
