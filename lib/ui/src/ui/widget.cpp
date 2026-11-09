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
  if (!g_ui_ctx.can_hover_current_window()) return false;

  auto const& wnd = g_ui_ctx.window();
  rc = intersect(rc, { 0, 0, wnd->width(), wnd->height() });
  if (rc.empty()) return false;

  auto p = g_ui_ctx.cursor_pos_on_window();
  // Actually, cursor position always need - 1, like 0~1919 in range 0~1920
  return rc.adjust_right_bottom(float2(-1)).contains(p);
}

auto button(size_t id, float x, float y, float width, float height, bool blocks_window_move = true) noexcept -> ButtonState
{
  g_ui_ctx.check_draw();
  err_if(g_ui_ctx.has_custom_transform(), "buttons cannot be used inside transform_beg()/transform_end()");
  assert(std::isfinite(x) && std::isfinite(y) && std::isfinite(width) && std::isfinite(height) && width > 0 && height > 0);

  // what is a button
  // button is a rectangle with width and height in specific position
  auto left_top     = float2{ x, y };
  auto right_bottom = float2{ x + width, y + height };
  auto widget_rect  = Rect{ left_top, right_bottom };
  g_ui_ctx.register_item(widget_rect);

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

  auto is_last_mouse_down_id = g_ui_ctx.is_last_mouse_down_id(id);
  if (rect.empty())
  {
    if (!is_last_mouse_down_id) return {};
  }
  else if (blocks_window_move)
    g_ui_ctx.add_move_invalid_area(widget_rect);

  auto mouse_left_button_state = get_key(Key::Mouse_Left_Button);

  // when cursor hover on it, it will change color to hovered color
  auto is_hovered  = g_ui_ctx.is_hover_on(id, left_top, right_bottom) && g_wnd_mgr.is_normal_cursor();
  auto is_move_out = g_ui_ctx.is_cursor_move_out(id);
  auto is_down     = is_last_mouse_down_id && mouse_left_button_state.has_down();
  if (is_hovered && (g_ui_ctx.is_last_mouse_up ? false : g_ui_ctx.mouse_down_window_pos.has_value()))
  {
    is_last_mouse_down_id = g_ui_ctx.add_mouse_left_button_state(id, left_top, right_bottom);
    is_down    = is_last_mouse_down_id && mouse_left_button_state.has_down();
    is_hovered = is_down                                                                         &&
                 !is_move_out                                                                    &&
                 Rect{ left_top, right_bottom }.contains(g_ui_ctx.mouse_down_window_pos.value()) &&
                 g_ui_ctx.mouse_down_window == g_ui_ctx.window()->handle();
  }

  auto clicked      = is_hovered && is_last_mouse_down_id && is_click_on(left_top, right_bottom) && !is_move_out;
  auto double_click = is_hovered && mouse_left_button_state.state == KeyState::double_click;
  auto dragged      = is_last_mouse_down_id && mouse_left_button_state.state == KeyState::up;

  return { clicked, double_click, is_hovered, is_move_out, is_down, dragged };
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

auto SliderConfig::CircleKnob(float bar_thickness, float radius, Color color, Color hover_color) noexcept -> DrawConfig
{
  return { bar_thickness, { radius, radius }, color, hover_color,
    [](float2 pos, float2 ext, Color col) { circle(pos + ext / 2, ext.x / 2, col); } };
}

auto SliderConfig::RectKnob(float bar_thickness, float2 ext, Color color, Color hover_color, float rounding) noexcept -> DrawConfig
{
  return { bar_thickness, ext, color, hover_color,
    [rounding](float2 pos, float2 ext, Color col) { rectangle(pos, pos + ext, col, {}, rounding); } };
}

auto slider(StringID name, float x, float y, float width, float height, float beg, float end, float& v, SliderConfig const& cfg) noexcept -> bool
{
  // promise valid arguments
  assert(std::isfinite(beg) && std::isfinite(end) && beg < end && width > 0 && height > 0);
  assert(cfg.vertical ? height > width : width > height);

  // set valid slider value
  v = std::clamp(v, beg, end);

  // main axis is the sliding direction, cross axis is the bar thickness direction
  auto main_len  = cfg.vertical ? height : width;
  auto cross_len = cfg.vertical ? width  : height;

  // get draw config
  auto knob_len         = 0.f;
  auto bar_thickness    = cross_len;
  auto has_knob         = cfg.draw_cfg.has_value();
  auto knob_color       = Color{};
  auto knob_hover_color = Color{};
  if (has_knob)
  {
    assert(cfg.draw_cfg->bar_thickness > 0 && cfg.draw_cfg->bar_thickness <= cross_len);
    assert(cfg.draw_cfg->knob_extent.x > 0 && cfg.draw_cfg->knob_extent.y > 0);
    assert((cfg.vertical ? cfg.draw_cfg->knob_extent.y : cfg.draw_cfg->knob_extent.x) < main_len);
    assert(cfg.draw_cfg->knob_draw);

    knob_len         = cfg.vertical ? cfg.draw_cfg->knob_extent.y : cfg.draw_cfg->knob_extent.x;
    bar_thickness    = cfg.draw_cfg->bar_thickness;
    knob_color       = cfg.draw_cfg->knob_color;
    knob_hover_color = cfg.draw_cfg->knob_hover_color;
  }

  // get base values
  auto ratio    = (v - beg) / (end - beg);
  auto len      = main_len - knob_len;
  auto offset   = len * ratio;
  auto left_top = cfg.vertical
    ? float2{ x + (width - bar_thickness) / 2, y + knob_len / 2 }
    : float2{ x + knob_len / 2, y + (height - bar_thickness) / 2 };
  auto rounding = cfg.rounding ? bar_thickness / 2 : 0.f;

  // default: horizontal grows left to right, vertical grows bottom to top; reverse flips it
  auto grow_from_min = cfg.vertical == cfg.reverse;
  auto get_knob_pos  = [&](float offset)
  {
    auto main_pos = grow_from_min ? offset : len - offset;
    return cfg.vertical ? float2{ x, y + main_pos } : float2{ x + main_pos, y };
  };

  // get bar and knob button states
  auto knob_pos   = get_knob_pos(offset);
  auto knob_state = ButtonState{};
  auto bar_state  = cfg.is_bar_clickable ? button(name, x, y, width, height) : ButtonState{};
  if (has_knob)
  {
    if (cfg.vertical)
      knob_pos.x = x + (width - cfg.draw_cfg->knob_extent.x) / 2;
    else
      knob_pos.y = y + (height - cfg.draw_cfg->knob_extent.y) / 2;
    knob_state = button(StringID{ hash(name, 0) }, knob_pos.x, knob_pos.y, cfg.draw_cfg->knob_extent.x, cfg.draw_cfg->knob_extent.y);
  }

  // get result color
  auto bar_color      = cfg.bar_color;
  auto progress_color = cfg.progress_color;
  if (bar_state.hovered)
  {
    bar_color = cfg.bar_hover_color;
    if (cfg.progress_hover_color.a)
      progress_color = cfg.progress_hover_color;
  }
  if (knob_state.hovered) knob_color = knob_hover_color;

  // process mouse down
  if (knob_state.down || bar_state.down)
  {
    knob_color = knob_hover_color;
    auto cursor    = get_cursor_pos_on_window();
    auto t         = cfg.vertical ? (cursor.y - y - knob_len / 2) / len : (cursor.x - x - knob_len / 2) / len;
    auto cur_ratio = std::clamp(grow_from_min ? t : 1.f - t, 0.f, 1.f);
    v = beg + cur_ratio * (end - beg);
    ratio  = cur_ratio;
    offset = len * ratio;
    auto pos = get_knob_pos(offset);
    if (cfg.vertical) knob_pos.y = pos.y;
    else              knob_pos.x = pos.x;
  }

  // draw progress
  auto right_bottom = left_top + (cfg.vertical ? float2{ bar_thickness, len } : float2{ len, bar_thickness });
  if (progress_color.a)
  {
    auto progress_len = len * ratio;
    rectangle(left_top, right_bottom, bar_color, {}, rounding);
    auto progress_lt = left_top;
    auto progress_rb = right_bottom;
    auto flags       = Flag<CornerFlag>{};
    if (cfg.vertical)
    {
      if (grow_from_min) { progress_rb.y = left_top.y + progress_len;     flags = CornerFlag::top;    }
      else               { progress_lt.y = right_bottom.y - progress_len; flags = CornerFlag::bottom; }
    }
    else
    {
      if (grow_from_min) { progress_rb.x = left_top.x + progress_len;     flags = CornerFlag::left;   }
      else               { progress_lt.x = right_bottom.x - progress_len; flags = CornerFlag::right;  }
    }
    rectangle(progress_lt, progress_rb, progress_color, {}, rounding, progress_len == len ? CornerFlag::all : flags);
  }
  else
    rectangle(left_top, right_bottom, bar_color, {}, rounding);

  // draw knob
  if (has_knob)
    cfg.draw_cfg->knob_draw(knob_pos, cfg.draw_cfg->knob_extent, knob_color);

  return knob_state.dragged || bar_state.dragged;
}

}
