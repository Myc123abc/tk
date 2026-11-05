#pragma once

#include "color.hpp"
#include "tk/string_id.hpp"

#include <functional>

namespace tk::ui {

struct ButtonState
{
  bool clicked{};
  bool double_click{};
  bool hovered{};
  bool move_out{};
  bool down{};

  constexpr operator bool() const noexcept
  {
    return clicked;
  }  
};

/**
 * a button feature can custom shape
 * @param name name cannot be duplicate in the window
 * @param x
 * @param y
 * @param width
 * @param height
 * @param blocks_window_move whether pressing the button prevents window dragging
 * @return button state
 */
auto button(StringID name, float x, float y, float width, float height, bool blocks_window_move = true) noexcept -> ButtonState;

/**
 * normal button
 * @param name name cannot be duplicate in the window
 * @param x
 * @param y
 * @param width
 * @param height
 * @param color
 * @param hover_color
 * @param click_color
 * @return button state
 */
auto button(
  StringID name,
  float    x,
  float    y,
  float    width,
  float    height,
  Color    color,
  Color    hover_color,
  Color    click_color = {}) noexcept -> ButtonState;

/**
 * draw a button, can draw an icon in the center of button
 * default have a color lerp animation when cursor hover on button and leave on it
 * @param name name cannot be duplicate in the window
 * @param x
 * @param y
 * @param width
 * @param height
 * @param button_color
 * @param button_hover_color
 * @param mouse_down_color
 * @param icon_update_func the function be called for draw icon by ui draw api,
 *                         Color is used for icon color lerp changed,
 * @param icon_width
 * @param icon_height
 * @param icon_color
 * @param icon_hover_color
 * @return button state
 */
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
  Color                                    icon_hover_color) noexcept -> ButtonState;

struct CheckBoxConfig
{
  float                thickness{ 4 };
  float                inset{ 2 };
  Color                color;
  std::optional<Color> inner_color;
  std::optional<Color> hover_color;
  std::optional<Color> hover_inner_color;
  std::optional<Color> click_color;
  std::optional<Color> click_inner_color;
  float                text_size{};
  Color                text_color;
  float                text_spacing{ 4 };
};

/**
 * draw a checkbox.
 * @param name
 * @param pos
 * @param extent
 * @param is_checked
 * @param cfg checkbox config
 * @return checked value
 */
auto check_box(StringID name, float2 pos, float extent, bool& is_checked, CheckBoxConfig const& cfg = {}) noexcept -> bool;

/**
 * Draw a checkbox with text on its right.
 * @param name name cannot be duplicate in the window
 * @param label displayed text
 * @param pos left top of the entire control
 * @param is_checked checked state
 * @param cfg checkbox config
 * @return extent of the checkbox and label
 */
auto check_box(StringID name, StringLiteral label, float2 pos, bool& is_checked, CheckBoxConfig const& cfg = {}) noexcept -> float2;

}
