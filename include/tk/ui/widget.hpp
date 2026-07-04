#pragma once

#include "color.hpp"

#include <string_view>
#include <functional>

namespace tk::ui {

////////////////////////////////////////////////////////////////////////////////
///                               Widget
////////////////////////////////////////////////////////////////////////////////

struct ButtonState
{
  bool clicked{};
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
 * @return button state
 */
auto button(std::string_view name, float x, float y, float width, float height) noexcept -> ButtonState;

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
  std::string_view name,
  float            x,
  float            y,
  float            width,
  float            height,
  Color            color,
  Color            hover_color,
  Color            click_color = {}) noexcept -> ButtonState;

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
  Color                                    icon_hover_color) noexcept -> ButtonState;

}
