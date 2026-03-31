#include "ui/ui.hpp"
#include "ui_context.hpp"
#include "text_engine.hpp"

#include <stb_image.h>

using namespace tk::renderer;

namespace tk::ui {

////////////////////////////////////////////////////////////////////////////////
///                              Misc
////////////////////////////////////////////////////////////////////////////////

void render() noexcept
{
	g_ui_ctx.render();
}

auto lerp(Color x, Color y, float v) noexcept -> glm::vec4
{
  return
  {
    std::lerp(x.r, y.r, v),
    std::lerp(x.g, y.g, v),
    std::lerp(x.b, y.b, v),
    std::lerp(x.a, y.a, v)
  };
}

auto lerp(glm::vec2 x, glm::vec2 y, float v) noexcept -> glm::vec2
{
  return { std::lerp(x.x, y.x, v), std::lerp(x.y, y.y, v) };
  // use round for small pixel level lerp for smooth
  // return { std::round(std::lerp(a.x, b.x, t)), std::round(std::lerp(a.y, b.y, t)) };
}

auto delta_time() noexcept -> double
{
  return g_ui_ctx.delta_time();
}

auto image_extent(std::string_view path) noexcept -> glm::vec2
{
  return g_img_mgr.extent(path);
}

void image(std::string_view path, glm::vec2 left_top, glm::vec2 right_bottom, uint8_t alpha) noexcept
{
  g_ui_ctx.image(path, left_top, right_bottom, alpha);
}

void load_font(std::string_view path) noexcept
{
  g_text_engine.load_font(path);
}

auto text(std::string_view text, glm::vec2 pos, float size, Color color, FontStyle style) noexcept -> glm::vec2
{
  return g_ui_ctx.text(text, pos, size, color, style, {});
}

auto text(std::string_view text, glm::vec2 pos, float size, Color inner_color, Color outer_color) noexcept -> glm::vec2
{
  return g_ui_ctx.text(text, pos, size, inner_color, {}, outer_color);
}

auto get_cursor_pos() noexcept -> glm::vec2
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

void add_move_invalid_area(glm::vec2 left_top, glm::vec2 right_bottom) noexcept
{
  g_ui_ctx.check_draw();
  if (g_ui_ctx.is_use_title_bar_now() && !g_ui_ctx.draw_title_bar)
  {
    left_top.y     += Title_Bar_Height;
    right_bottom.y += Title_Bar_Height;
  }

  auto scale = g_ui_ctx.get_scale();
  left_top     *= scale;
  right_bottom *= scale;

  g_ui_ctx._window->add_move_invald_areas(
  {
    static_cast<LONG>(left_top.x),
    static_cast<LONG>(left_top.y),
    static_cast<LONG>(right_bottom.x),
    static_cast<LONG>(right_bottom.y)
  });
}

auto window_extent() noexcept -> glm::vec2
{
  g_ui_ctx.check_draw();
  return glm::vec2{ g_ui_ctx._window->snap.width, g_ui_ctx._window->snap.height } / g_ui_ctx.get_scale();
}

auto window_drawable_extent() noexcept -> glm::vec2
{
  auto extent = window_extent();
  if (g_ui_ctx.is_use_title_bar_now())
    extent.y -= Title_Bar_Height;
  return extent;
}

auto is_fullscreen_window() noexcept -> bool
{
  g_ui_ctx.check_draw();
  return g_ui_ctx._window->snap.fullscreen_window;
}

void fullscreen_window() noexcept
{
  g_ui_ctx.check_draw();
  if (g_ui_ctx._window->cfg.no_resize) return;
  g_ui_ctx.fullscreen_window();
}

void restore_fullscreen_window() noexcept
{
  g_ui_ctx.check_draw();
  if (g_ui_ctx._window->cfg.no_resize) return;
  g_ui_ctx.restore_fullscreen_window();
}


////////////////////////////////////////////////////////////////////////////////
///                            Shape Operator
////////////////////////////////////////////////////////////////////////////////

void discard_rectangle(glm::vec2 left_top, glm::vec2 right_bottom) noexcept
{
  g_ui_ctx.check_draw();  
  g_ui_ctx.check_path_not_draw();
  g_ui_ctx.check_union_not_draw();

  auto offset = g_ui_ctx.get_render_pos();
  left_top     += offset;
  right_bottom += offset;

  auto scale = g_ui_ctx.get_scale();
  left_top     *= scale;
  right_bottom *= scale;

  g_ui_ctx.cmd()->add_discard_rectangle(left_top, right_bottom);
}

void begin_path() noexcept
{
  g_ui_ctx.begin_path();
}

void end_path(Color color, float thickness) noexcept
{
  g_ui_ctx.end_path(color, thickness);
}

void begin_union() noexcept
{
  g_ui_ctx.begin_union();
}

void end_union(Color color, float thickness) noexcept
{
  g_ui_ctx.end_union(color, thickness);
}

////////////////////////////////////////////////////////////////////////////////
///                            Geometry
////////////////////////////////////////////////////////////////////////////////

void rectangle(glm::vec2 left_top, glm::vec2 right_bottom, Color color, float thickness) noexcept
{
	g_ui_ctx.check_draw();
	g_ui_ctx.check_path_not_draw();

	auto offset = g_ui_ctx.get_render_pos();
	left_top     += offset;
	right_bottom += offset;

  auto scale = g_ui_ctx.get_scale();
  left_top     *= scale;
  right_bottom *= scale;

  g_ui_ctx.cmd()->draw_rectangle(left_top, right_bottom, color, thickness);
}

void triangle(glm::vec2 p0, glm::vec2 p1, glm::vec2 p2, Color color, float thickness) noexcept
{
	g_ui_ctx.check_draw();
	g_ui_ctx.check_path_not_draw();

	auto offset = g_ui_ctx.get_render_pos();
	p0 += offset;
	p1 += offset;
	p2 += offset;

  auto scale = g_ui_ctx.get_scale();
  p0 *= scale;
  p1 *= scale;
  p2 *= scale;

	g_ui_ctx.cmd()->draw_triangle(p0, p1, p2, color, thickness);
}

void circle(glm::vec2 center, float radius, Color color, float thickness) noexcept
{
	g_ui_ctx.check_draw();
	g_ui_ctx.check_path_not_draw();

	auto offset = g_ui_ctx.get_render_pos();
  center += offset;

  auto scale = g_ui_ctx.get_scale();
  radius *= scale;
  center *= scale;

  g_ui_ctx.cmd()->draw_circle(center, radius, color, thickness);
}

void line(glm::vec2 p0, glm::vec2 p1, Color color) noexcept
{
  if (p0 == p1) return;

	g_ui_ctx.check_draw();

	auto offset = g_ui_ctx.get_render_pos();
  p0 += offset;
  p1 += offset;

  auto scale = g_ui_ctx.get_scale();
  p0 *= scale;
  p1 *= scale;

  g_ui_ctx.cmd()->draw_line(p0, p1, color);
}

void bezier(glm::vec2 p0, glm::vec2 p1, glm::vec2 p2, Color color) noexcept
{
	g_ui_ctx.check_draw();

	auto offset = g_ui_ctx.get_render_pos();
  p0 += offset;
  p1 += offset;
  p2 += offset;

  auto scale = g_ui_ctx.get_scale();
  p0 *= scale;
  p1 *= scale;
  p2 *= scale;

  g_ui_ctx.cmd()->draw_bezier(p0, p1, p2, color);
}

////////////////////////////////////////////////////////////////////////////////
///                               Widget
////////////////////////////////////////////////////////////////////////////////

auto is_hover_on(glm::vec2 left_top, glm::vec2 right_bottom) noexcept -> bool
{
  g_ui_ctx.check_draw();
  auto p = g_ui_ctx._window->cursor_pos();
  return g_ui_ctx.cursor_on_window == g_ui_ctx._window->snap.handle &&
         !g_ui_ctx._window->snap.move_from_maximize                 &&
         point_on(p, left_top, right_bottom);
}

auto is_click_on(glm::vec2 left_top, glm::vec2 right_bottom) noexcept -> bool
{
  g_ui_ctx.check_draw();
  auto window = g_ui_ctx._window;
  if (!window->is_active()             ||
		   window->is_moving_or_resizing() ||
      !g_ui_ctx.mouse_down_pos         ||
      !g_ui_ctx.mouse_up_pos           ||
       g_ui_ctx.mouse_down_window != g_ui_ctx.mouse_up_window) return false;
  return !g_ui_ctx.is_move_from_maximize                                   &&
         point_on(g_ui_ctx.mouse_down_pos.value(), left_top, right_bottom) &&
         point_on(g_ui_ctx.mouse_up_pos.value(),   left_top, right_bottom);
}

auto lerp_ping_pong(std::string_view name, bool b, double duration) noexcept -> double
{
  return g_ui_ctx.lerp_ping_pong(b, g_ui_ctx.generic_id(name), duration);
}

void reset_lerpolator(std::string_view name) noexcept
{
  g_ui_ctx.reset_lerpolator(g_ui_ctx.get_id(name));
}

auto button(size_t id, int x, int y, uint32_t width, uint32_t height) noexcept-> ButtonState
{
  g_ui_ctx.check_draw();
  g_ui_ctx.check_path_not_draw();
  g_ui_ctx.check_union_not_draw();

  // what is a button
  // button is a rectangle with width and height in specific position
  auto left_top     = glm::vec2{ x, y };
  auto right_bottom = glm::vec2{ x + width, y + height };
  if (g_ui_ctx.is_use_title_bar_now() && !g_ui_ctx.draw_title_bar)
  {
    left_top.y     += Title_Bar_Height;
    right_bottom.y += Title_Bar_Height;
  }

  auto scale = g_ui_ctx.get_scale();
  left_top     *= scale;
  right_bottom *= scale;

  // when cursor hover on it, it will change color to hovered color
  auto is_hovered  = g_ui_ctx.is_hover_on(id, left_top, right_bottom);
  auto is_move_out = g_ui_ctx.is_cursor_move_out(id);
  if (is_hovered && g_ui_ctx.mouse_down_pos)
  {
    g_ui_ctx.add_mouse_left_button_state(id, left_top, right_bottom);
    is_hovered = !is_move_out                                                      &&
                 point_on(g_ui_ctx.mouse_down_pos.value(), left_top, right_bottom) &&
                 g_ui_ctx.mouse_down_window == g_ui_ctx._window->snap.handle;
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
  std::string_view                        name,
  int                                     x,
  int                                     y,
  uint32_t                                width,
  uint32_t                                height,
  Color                                   button_color,
  Color                                   button_hover_color,
  std::optional<Color>                    mouse_down_color,
  std::function<void(uint32_t, uint32_t)> icon_update_func,
  uint32_t                                icon_width,
  uint32_t                                icon_height,
  Color                                   icon_color,
  Color                                   icon_hover_color) noexcept-> ButtonState
{
  auto id    = g_ui_ctx.generic_id(name);
  auto state = button(id, x, y, width, height);

  auto value = g_ui_ctx.lerp_ping_pong(state.hovered, id, 200'000);
  if (mouse_down_color && state.move_out) button_hover_color = mouse_down_color.value();
  button_color = lerp(button_color, button_hover_color, value);

  // when mouse down, color also change
  if (mouse_down_color && state.hovered)
  {
    auto state = g_ui_ctx.get_key(Key::Mouse_Left_Button);
    if (has_flag(state, KeyState::down) || state == KeyState::press)
      button_color = mouse_down_color.value();
  }

  // draw button
  ui::rectangle({ x, y }, { x + width, y + height }, button_color);

  // draw icon
  if (icon_update_func)
  {
    icon_color = lerp(icon_color, icon_hover_color, value);
    auto x_offset = (width  - icon_width)  / 2;
    auto y_offset = (height - icon_height) / 2;
    
    g_ui_ctx.render_on(x + x_offset, y + y_offset, [&]
    {
      g_ui_ctx.cmd()->enable_tmp_color(icon_color);
      icon_update_func(icon_width, icon_height);
      g_ui_ctx.cmd()->disable_tmp_color();
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
  if (g_ui_ctx._window->is_active())
    return { g_ui_ctx.get_key(key) };
  return {};
}

}
