#include "ui/widget.hpp"
#include "ui_context.hpp"
#include "../renderer/window/window_manager.hpp"

using namespace tk;
using namespace tk::ui;
using namespace tk::renderer;

namespace {

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

}

namespace tk::ui {

auto is_hover_on(float2 left_top, float2 right_bottom) noexcept -> bool
{
  g_ui_ctx.check_draw();
  auto p = g_ui_ctx.window()->cursor_pos();
  return g_ui_ctx.cursor_on_window == g_ui_ctx.window()->handle() &&
         !g_ui_ctx.window()->is_move_from_maximize()              &&
         Rect{ left_top, right_bottom }.contains(p);
}

auto button(size_t id, float x, float y, float width, float height) noexcept -> ButtonState
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

  return { is_hovered && is_click_on(left_top, right_bottom), is_hovered, is_move_out };
}

auto button(std::string_view name, float x, float y, float width, float height) noexcept -> ButtonState
{
  return button(g_ui_ctx.generic_id(name), x, y, width, height);
}

auto button(
  std::string_view name,
  float            x,
  float            y,
  float            width,
  float            height,
  Color            color,
  Color            hover_color,
  Color            click_color) noexcept -> ButtonState
{
  auto id    = g_ui_ctx.generic_id(name);
  auto state = button(id, x, y, width, height);

  auto value = g_ui_ctx.ping_pong(state.hovered, id, 0, 20'000);
  if (click_color.a && state.move_out) g_ui_ctx.reset_tween(id);
  color = lerp(color, hover_color, value);

  if (click_color.a && state.hovered)
  {
    auto state = g_ui_ctx.get_key(Key::Mouse_Left_Button);
    if (state.contains(KeyState::down) || state == KeyState::press)
      color = click_color;
  }
  ui::rectangle({ x, y }, { x + width, y + height }, color);
  return state;
}

auto button(
  std::string_view                         name,
  float                                    x,
  float                                    y,
  float                                    width,
  float                                    height,
  Color                                    button_color,
  Color                                    button_hover_color,
  Color                                    mouse_down_color,
  std::function<void(float, float, Color)> icon_update_func,
  float                                    icon_width,
  float                                    icon_height,
  Color                                    icon_color,
  Color                                    icon_hover_color) noexcept -> ButtonState
{
  auto id    = g_ui_ctx.generic_id(name);
  auto state = button(id, x, y, width, height);

  auto value = g_ui_ctx.ping_pong(state.hovered, id, 200'000);
  if (mouse_down_color.a && state.move_out) g_ui_ctx.reset_tween(id);
  button_color = lerp(button_color, button_hover_color, value);

  // when mouse down, color also change
  if (mouse_down_color.a && state.hovered)
  {
    auto state = g_ui_ctx.get_key(Key::Mouse_Left_Button);
    if (state.contains(KeyState::down) || state == KeyState::press)
      button_color = mouse_down_color;
  }

  // draw button
  ui::rectangle({ x, y }, { x + width, y + height }, button_color);

  // draw icon
  if (icon_update_func)
  {
    icon_color = lerp(icon_color, icon_hover_color, value);
    auto x_offset = (width  - icon_width)  / 2;
    auto y_offset = (height - icon_height) / 2;

    auto scale  = g_ui_ctx.window()->scale();
    auto icon_x = std::round((x + x_offset) * scale) / scale;
    auto icon_y = std::round((y + y_offset) * scale) / scale;

    g_ui_ctx.render_on(icon_x, icon_y, [&]
    {
      icon_update_func(icon_width, icon_height, icon_color);
    });
  }

  return state;
}

}
