#include "ui/ui.hpp"
#include "ui_context.hpp"
#include "text_engine.hpp"
#include "../renderer/window/window_manager.hpp"

#include <stb_image.h>

using namespace tk::renderer;

namespace {

auto intersect_rect(RECT lhs, RECT rhs) noexcept -> std::optional<RECT>
{
  auto res = RECT{};
  if (IntersectRect(&res, &lhs, &rhs))
    return res;
  return {};
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

auto image(std::string_view path, float2 left_top, float2 right_bottom, uint8_t alpha) noexcept -> bool
{
  return g_ui_ctx.image(path, left_top, right_bottom, alpha);
}

void load_image(std::string_view path) noexcept
{
  g_img_mgr.try_load(path);
}

void load_font(std::string_view path) noexcept
{
  g_text_engine.load_font(path);
}

auto text(std::string_view text, float2 pos, float size, Color color, FontStyle style) noexcept -> float2
{
  return g_ui_ctx.text(text, pos, size, color, style, {});
}

auto text(std::string_view text, float2 pos, float size, Color inner_color, Color outer_color) noexcept -> float2
{
  return g_ui_ctx.text(text, pos, size, inner_color, {}, outer_color);
}

auto get_cursor_pos() noexcept -> float2
{
  return renderer::get_cursor_pos();
}

////////////////////////////////////////////////////////////////////////////////
///                             Window
////////////////////////////////////////////////////////////////////////////////

void begin(std::string_view name, int x, int y, uint32_t width, uint32_t height, bool* is_closed, WindowConfig const& cfg) noexcept
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


////////////////////////////////////////////////////////////////////////////////
///                            Shape Operator
////////////////////////////////////////////////////////////////////////////////

void discard_beg(std::function<void()> func) noexcept
{
  // g_ui_ctx.frame_data()->discard_beg(func);
}

void discard_end() noexcept
{
  // g_ui_ctx.frame_data()->discard_end();
}

void union_beg() noexcept
{
  // g_ui_ctx.frame_data()->union_beg();
}

void union_end() noexcept
{
  // g_ui_ctx.frame_data()->union_end();
}

////////////////////////////////////////////////////////////////////////////////
///                            Geometry
////////////////////////////////////////////////////////////////////////////////

void rectangle(float2 left_top, float2 right_bottom, Color color, float thickness) noexcept
{
	g_ui_ctx.check_draw();

	auto offset = g_ui_ctx.get_render_pos();
	left_top     += offset;
	right_bottom += offset;

  auto scale = g_ui_ctx.window()->scale();
  left_top     *= scale;
  right_bottom *= scale;
  thickness    *= scale;

  if (g_ui_ctx.window()->is_resizing())
  {
    offset = g_ui_ctx.window()->real_pos();
    left_top     += offset;
    right_bottom += offset;
  }

  left_top     = floor(left_top)     + .5f;
  right_bottom = floor(right_bottom) - .5f;

  g_ui_ctx.frame_data()->add_rect(left_top, right_bottom, color, thickness);
}

void triangle(float2 p0, float2 p1, float2 p2, Color color, float thickness) noexcept
{
	g_ui_ctx.check_draw();

	auto offset = g_ui_ctx.get_render_pos();
	p0 += offset;
	p1 += offset;
	p2 += offset;

  auto scale = g_ui_ctx.window()->scale();
  p0        *= scale;
  p1        *= scale;
  p2        *= scale;
  thickness *= scale;

  if (g_ui_ctx.window()->is_resizing())
  {
    offset = g_ui_ctx.window()->real_pos();
    p0 += offset;
    p1 += offset;
    p2 += offset;
  }

	g_ui_ctx.frame_data()->add_triangle(p0, p1, p2, color, thickness);
}

void circle(float2 center, float radius, Color color, float thickness) noexcept
{
	g_ui_ctx.check_draw();

	auto offset = g_ui_ctx.get_render_pos();
  center += offset;

  auto scale = g_ui_ctx.window()->scale();
  radius    *= scale;
  center    *= scale;
  thickness *= scale;

  if (g_ui_ctx.window()->is_resizing())
  {
    offset = g_ui_ctx.window()->real_pos();
    center += offset;
  }

  g_ui_ctx.frame_data()->add_circle(center, radius, color, thickness);
}

void line(float2 p0, float2 p1, Color color, float thickness) noexcept
{
  if (p0 == p1) return;

  if (thickness < 1) thickness = 1;

	g_ui_ctx.check_draw();

	auto offset = g_ui_ctx.get_render_pos();
  p0 += offset;
  p1 += offset;

  auto scale = g_ui_ctx.window()->scale();
  p0        *= scale;
  p1        *= scale;
  thickness *= scale;

  if (g_ui_ctx.window()->is_resizing())
  {
    offset = g_ui_ctx.window()->real_pos();
    p0 += offset;
    p1 += offset;
  }

  p0 = floor(p0) + .5f;
  p1 = floor(p1) + .5f;

  g_ui_ctx.frame_data()->add_line(p0, p1, color, thickness);
}

void bezier_quad(float2 p0, float2 p1, float2 p2, Color color, float thickness) noexcept
{
  if (thickness < 1) thickness = 1;

	g_ui_ctx.check_draw();

	auto offset = g_ui_ctx.get_render_pos();
  p0 += offset;
  p1 += offset;
  p2 += offset;

  auto scale = g_ui_ctx.window()->scale();
  p0        *= scale;
  p1        *= scale;
  p2        *= scale;
  thickness *= scale;

  if (g_ui_ctx.window()->is_resizing())
  {
    offset = g_ui_ctx.window()->real_pos();
    p0 += offset;
    p1 += offset;
    p2 += offset;
  }

  g_ui_ctx.frame_data()->add_bezier_quad(p0, p1, p2, color, thickness);
}

void bezier_cubic(float2 p0, float2 p1, float2 p2, float2 p3, Color color, float thickness) noexcept
{
  if (thickness < 1) thickness = 1;

	g_ui_ctx.check_draw();

	auto offset = g_ui_ctx.get_render_pos();
  p0 += offset;
  p1 += offset;
  p2 += offset;
  p3 += offset;

  auto scale = g_ui_ctx.window()->scale();
  p0        *= scale;
  p1        *= scale;
  p2        *= scale;
  p3        *= scale;
  thickness *= scale;

  if (g_ui_ctx.window()->is_resizing())
  {
    offset = g_ui_ctx.window()->real_pos();
    p0 += offset;
    p1 += offset;
    p2 += offset;
    p3 += offset;
  }

  g_ui_ctx.frame_data()->add_bezier_cubic(p0, p1, p2, p3, color, thickness);
}

////////////////////////////////////////////////////////////////////////////////
///                               Path
////////////////////////////////////////////////////////////////////////////////

void path_begin() noexcept
{
  g_ui_ctx.check_draw();
  g_ui_ctx.frame_data()->path_begin();
}

void path_line(float2 p0, float2 p1) noexcept
{
	g_ui_ctx.check_draw();

	auto offset = g_ui_ctx.get_render_pos();
  p0 += offset;
  p1 += offset;

  auto scale = g_ui_ctx.window()->scale();
  p0 *= scale;
  p1 *= scale;

  if (g_ui_ctx.window()->is_resizing())
  {
    offset = g_ui_ctx.window()->real_pos();
    p0 += offset;
    p1 += offset;
  }

  g_ui_ctx.frame_data()->add_path_line(p0, p1);
}

void path_arc(float2 center, float radius, float min, float max) noexcept
{
  g_ui_ctx.check_draw();

	auto offset = g_ui_ctx.get_render_pos();
  center += offset;

  auto scale = g_ui_ctx.window()->scale();
  center *= scale;
  radius *= scale;

  if (g_ui_ctx.window()->is_resizing())
  {
    offset = g_ui_ctx.window()->real_pos();
    center += offset;
  }

  g_ui_ctx.frame_data()->add_path_arc(center, radius, min, max);
}

void path_bezier_quad(float2 p0, float2 p1, float2 p2) noexcept
{
  g_ui_ctx.check_draw();

  auto offset = g_ui_ctx.get_render_pos();
  p0 += offset;
  p1 += offset;
  p2 += offset;

  auto scale = g_ui_ctx.window()->scale();
  p0 *= scale;
  p1 *= scale;
  p2 *= scale;

  if (g_ui_ctx.window()->is_resizing())
  {
    offset = g_ui_ctx.window()->real_pos();
    p0 += offset;
    p1 += offset;
    p2 += offset;
  }

  g_ui_ctx.frame_data()->add_path_bezier_quad(p0, p1, p2);
}

void path_bezier_cubic(float2 p0, float2 p1, float2 p2, float2 p3) noexcept
{
	g_ui_ctx.check_draw();

	auto offset = g_ui_ctx.get_render_pos();
  p0 += offset;
  p1 += offset;
  p2 += offset;
  p3 += offset;

  auto scale = g_ui_ctx.window()->scale();
  p0 *= scale;
  p1 *= scale;
  p2 *= scale;
  p3 *= scale;

  if (g_ui_ctx.window()->is_resizing())
  {
    offset = g_ui_ctx.window()->real_pos();
    p0 += offset;
    p1 += offset;
    p2 += offset;
    p3 += offset;
  }

  g_ui_ctx.frame_data()->add_path_bezier_cubic(p0, p1, p2, p3);
}

void path_end(Color color, float thickness) noexcept
{
	g_ui_ctx.check_draw();
  auto scale = g_ui_ctx.window()->scale();
  thickness *= scale;
  g_ui_ctx.frame_data()->path_end(color, thickness);
}

////////////////////////////////////////////////////////////////////////////////
///                               Widget
////////////////////////////////////////////////////////////////////////////////

auto is_hover_on(float2 left_top, float2 right_bottom) noexcept -> bool
{
  g_ui_ctx.check_draw();
  auto p = g_ui_ctx.window()->cursor_pos();
  return g_ui_ctx.cursor_on_window == g_ui_ctx.window()->handle() &&
         !g_ui_ctx.window()->is_move_from_maximize()              &&
         Rect{ left_top, right_bottom }.contains(p);
}

auto is_click_on(float2 left_top, float2 right_bottom) noexcept -> bool
{
  g_ui_ctx.check_draw();
  auto window = g_ui_ctx.window();
  if (!window->is_active()             ||
		   window->is_moving_or_resizing() ||
      !g_ui_ctx.mouse_down_pos         ||
      !g_ui_ctx.mouse_up_pos           ||
       g_ui_ctx.mouse_down_window != g_ui_ctx.mouse_up_window) return false;
  auto rc = Rect{ left_top, right_bottom };
  return !g_ui_ctx.is_move_from_maximize               &&
          rc.contains(g_ui_ctx.mouse_down_pos.value()) &&
          rc.contains(g_ui_ctx.mouse_up_pos.value());
}

auto ping_pong(std::string_view name, bool b, double duration, Tween::Ease ease) noexcept -> double
{
  return g_ui_ctx.ping_pong(b, g_ui_ctx.generic_id(name), duration, ease);
}

void reset_tween(std::string_view name) noexcept
{
  g_ui_ctx.reset_tween(g_ui_ctx.get_id(name));
}

auto button(size_t id, int x, int y, uint32_t width, uint32_t height) noexcept-> ButtonState
{
  g_ui_ctx.check_draw();

  // what is a button
  // button is a rectangle with width and height in specific position
  auto left_top     = float2{ x, y };
  auto right_bottom = float2{ x + width, y + height };
  if (g_ui_ctx.is_use_title_bar_now() && !g_ui_ctx.draw_title_bar)
  {
    left_top.y     += Title_Bar_Height;
    right_bottom.y += Title_Bar_Height;
  }

  auto scale = g_ui_ctx.window()->scale();
  left_top     *= scale;
  right_bottom *= scale;

  // when cursor hover on it, it will change color to hovered color
  auto is_hovered  = g_ui_ctx.is_hover_on(id, left_top, right_bottom) && g_wnd_mgr.is_normal_cursor();
  auto is_move_out = g_ui_ctx.is_cursor_move_out(id);
  if (is_hovered && g_ui_ctx.mouse_down_pos)
  {
    g_ui_ctx.add_mouse_left_button_state(id, left_top, right_bottom);
    is_hovered = !is_move_out                                                              &&
                  Rect{ left_top, right_bottom }.contains(g_ui_ctx.mouse_down_pos.value()) &&
                  g_ui_ctx.mouse_down_window == g_ui_ctx.window()->handle();
  }

  return { is_hovered && ui::is_click_on(left_top, right_bottom), is_hovered, is_move_out };
}

auto button(std::string_view name, int x, int y, uint32_t width, uint32_t height) noexcept-> ButtonState
{
  return button(g_ui_ctx.generic_id(name), x, y, width, height);
}

auto button(
  std::string_view name,
  int              x,
  int              y,
  uint32_t         width,
  uint32_t         height,
  Color            button_color,
  Color            button_hover_color) noexcept-> ButtonState
{
  auto state = button(name, x, y, width, height);
  ui::rectangle({ x, y }, { x + width, y + height }, state.hovered ? button_hover_color : button_color);
  return state;
}

auto button(
  std::string_view                               name,
  int                                            x,
  int                                            y,
  uint32_t                                       width,
  uint32_t                                       height,
  Color                                          button_color,
  Color                                          button_hover_color,
  std::optional<Color>                           mouse_down_color,
  std::function<void(uint32_t, uint32_t, Color)> icon_update_func,
  uint32_t                                       icon_width,
  uint32_t                                       icon_height,
  Color                                          icon_color,
  Color                                          icon_hover_color) noexcept-> ButtonState
{
  auto id    = g_ui_ctx.generic_id(name);
  auto state = button(id, x, y, width, height);

  auto value = g_ui_ctx.ping_pong(state.hovered, id, 200'000);
  if (mouse_down_color && state.move_out) g_ui_ctx.reset_tween(id);
  button_color = lerp(button_color, button_hover_color, value);

  // when mouse down, color also change
  if (mouse_down_color && state.hovered)
  {
    auto state = g_ui_ctx.get_key(Key::Mouse_Left_Button);
    if (has_flag(state, KeyState::down) || state == KeyState::press)
      button_color = mouse_down_color.value();
  }

  // draw button
  ui::rectangle({ x, y }, { x + static_cast<int>(width), y + static_cast<int>(height) }, button_color);

  // draw icon
  if (icon_update_func)
  {
    icon_color = lerp(icon_color, icon_hover_color, value);
    auto x_offset = (width  - icon_width)  / 2;
    auto y_offset = (height - icon_height) / 2;
    
    g_ui_ctx.render_on(x + x_offset, y + y_offset, [&]
    {
      icon_update_func(icon_width, icon_height, icon_color);
    });
  }

  return state;
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
