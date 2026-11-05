#include "ui/widget.hpp"
#include "ui_context.hpp"
#include "../renderer/window/window_manager.hpp"
#include "input_system.hpp"

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
      !g_ui_ctx.mouse_down_window_pos  ||
      !g_ui_ctx.mouse_up_window_pos    ||
       g_ui_ctx.mouse_down_window != g_ui_ctx.mouse_up_window) return false;
  auto rc = Rect{ left_top, right_bottom };
  return !g_ui_ctx.is_move_from_maximize               &&
          rc.contains(g_ui_ctx.mouse_down_window_pos.value()) &&
          rc.contains(g_ui_ctx.mouse_up_window_pos.value());
}

void draw_check_box(float2 pos, float extent, Color color, bool is_checked, ButtonState const& state, CheckBoxConfig const& cfg) noexcept
{
  auto outer_color = state.hovered ? cfg.hover_color.value_or(color) : color;
  if (state.hovered && state.down)
    outer_color = cfg.click_color.value_or(outer_color);
  rectangle(pos, pos + extent, outer_color, cfg.thickness);

  if (!is_checked) return;

  auto thickness   = cfg.thickness + cfg.inset;
  auto inner_color = cfg.inner_color.value_or(color);
  if (state.hovered)
    inner_color = state.down
      ? cfg.click_inner_color.value_or(inner_color)
      : cfg.hover_inner_color.value_or(inner_color);
  rectangle(pos + thickness, pos + extent - thickness, inner_color);
}

}

namespace tk::ui {

auto is_hover_on(Rect rc) noexcept -> bool
{
  g_ui_ctx.check_draw();
  auto const& wnd = g_ui_ctx.window();
  auto        p   = wnd->cursor_pos();
  return g_ui_ctx.cursor_on_window == wnd->handle()     &&
         !wnd->is_move_from_maximize()                  &&
         wnd->rect().contains(p + wnd->pos())           &&
          // Actually, cursor position always need - 1, like 0~1919 in range 0~1920
         rc.adjust_right_bottom(float2(-1)).contains(p);
}

auto button(size_t id, float x, float y, float width, float height, bool blocks_window_move = true) noexcept -> ButtonState
{
  g_ui_ctx.check_draw();

  // what is a button
  // button is a rectangle with width and height in specific position
  auto left_top     = float2{ x, y };
  auto right_bottom = float2{ x + width, y + height };
  if (blocks_window_move)
    g_ui_ctx.add_move_invalid_area({ left_top, right_bottom });

  if (g_ui_ctx.is_use_title_bar_now() && !g_ui_ctx.draw_title_bar)
  {
    left_top.y     += Title_Bar_Height;
    right_bottom.y += Title_Bar_Height;
  }

  auto scale = g_ui_ctx.window()->scale();
  left_top     *= scale;
  right_bottom *= scale;
  auto rect = g_ui_ctx.transform_widget_rect({ left_top, right_bottom });
  left_top     = rect.left_top();
  right_bottom = rect.right_bottom();

  auto is_last_mouse_down_id  = g_ui_ctx.is_last_mouse_down_id(id);
  auto mouse_left_button_state = get_key(Key::Mouse_Left_Button);

  // when cursor hover on it, it will change color to hovered color
  auto is_hovered  = g_ui_ctx.is_hover_on(id, left_top, right_bottom) && g_wnd_mgr.is_normal_cursor();
  auto is_move_out = g_ui_ctx.is_cursor_move_out(id);
  auto is_down     = is_last_mouse_down_id && mouse_left_button_state.has_down();
  if (is_hovered && (g_ui_ctx.is_last_mouse_up ? false : g_ui_ctx.mouse_down_window_pos.has_value()))
  {
    g_ui_ctx.add_mouse_left_button_state(id, left_top, right_bottom);
    is_hovered = is_down                                                                  &&
                 !is_move_out                                                             &&
                 Rect{ left_top, right_bottom }.contains(g_ui_ctx.mouse_down_window_pos.value()) &&
                 g_ui_ctx.mouse_down_window == g_ui_ctx.window()->handle();
  }

  auto clicked      = is_hovered && is_last_mouse_down_id && is_click_on(left_top, right_bottom);
  auto double_click = is_hovered && mouse_left_button_state.state == KeyState::double_click;

  return { clicked, double_click, is_hovered, is_move_out, is_down };
}

auto button(StringID name, float x, float y, float width, float height, bool blocks_window_move) noexcept -> ButtonState
{
  return button(g_ui_ctx.generic_id(name), x, y, width, height, blocks_window_move);
}

auto button(
  StringID name,
  float    x,
  float    y,
  float    width,
  float    height,
  Color    color,
  Color    hover_color,
  Color    click_color) noexcept -> ButtonState
{
  auto id    = g_ui_ctx.generic_id(name);
  auto state = button(id, x, y, width, height);

  auto value = g_ui_ctx.ping_pong(state.hovered, id, 0, 20'000);
  if (click_color.a && state.move_out) g_ui_ctx.reset_tween(id);
  color = lerp(color, hover_color, value);

  if (click_color.a && state.hovered)
  {
    auto state = g_input_sys[Key::Mouse_Left_Button];
    if (state.contains(KeyState::down) || state == KeyState::press)
      color = click_color;
  }
  ui::rectangle({ x, y }, { x + width, y + height }, color);
  return state;
}

auto button(
  StringID                                 name,
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
    auto state = g_input_sys[Key::Mouse_Left_Button];
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

auto check_box(StringID name, float2 pos, float extent, bool& is_checked, CheckBoxConfig const& cfg) noexcept -> bool
{
  auto state = button(name, pos.x, pos.y, extent, extent);
  if (state) is_checked = !is_checked;
  draw_check_box(pos, extent, cfg.color, is_checked, state, cfg);
  return is_checked;
}

auto check_box(StringID name, StringLiteral label, float2 pos, bool& is_checked, CheckBoxConfig const& cfg) noexcept -> float2
{
  auto text_extent = ui::text(label, cfg.text_size).extent;
  check_box(name, pos, text_extent.y, is_checked, cfg);
  ui::text(label, pos + float2{ text_extent.y + cfg.text_spacing, 0 }, cfg.text_size, cfg.text_color);
  return { text_extent.y + cfg.text_spacing + text_extent.x, text_extent.y };
}

}
