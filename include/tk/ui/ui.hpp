#pragma once

#include "../window/type.hpp"

#include <string_view>
#include <optional>

#include <glm/glm.hpp>

namespace tk { namespace ui {

////////////////////////////////////////////////////////////////////////////////
///                                Misc
////////////////////////////////////////////////////////////////////////////////

struct Color
{
  Color() = default;

  Color(uint32_t color) noexcept
  {
    r = static_cast<float>((color >> 24) & 0xFF) / 255;
    g = static_cast<float>((color >> 16) & 0xFF) / 255;
    b = static_cast<float>((color >> 8 ) & 0xFF) / 255;
    a = static_cast<float>((color      ) & 0xFF) / 255;
  }

  Color(glm::vec4 color) noexcept
    : r(color.r), g(color.g), b(color.b), a(color.a) {}

  operator glm::vec4() noexcept { return { r, g, b, a }; }

  float r{}, g{}, b{}, a{};
};

/**
 * get lerp color
 * @param x begin of color
 * @param y end of color
 * @param v lerp value
 * @return lerp color
 */
auto color_lerp(Color x, Color y, float v) noexcept -> glm::vec4;

// render windows
void render() noexcept;

struct WindowConfig
{
  bool display_title_bar{};
};

/**
 * get frame delta time
 * @return delta time(us)
 */
auto delta_time() noexcept -> double;

////////////////////////////////////////////////////////////////////////////////
///                             Window
////////////////////////////////////////////////////////////////////////////////

/**
 * begin a window
 * @param name window name cannot be duplicated
 * @param x
 * @param y
 * @param width
 * @param height
 * @param is_closed make the window can be closed if is_closed is not nullptr,
 *                  and this is only a flag to indicate whether the window is closed,
 *                  if you want to close the window, stop call the begin and end of this window
 * @param cfg window config
 */
void begin(std::string_view name, int x, int y, uint32_t width, uint32_t height, bool* is_closed, WindowConfig cfg = {}) noexcept;

// end a window
void end() noexcept;

/**
 * get mouse state of current window
 * @return mouse state
 */
auto get_mouse_state() -> window::MouseState;

////////////////////////////////////////////////////////////////////////////////
///                            Shape Operator
////////////////////////////////////////////////////////////////////////////////

/**
 * set render position
 * @param x
 * @param y
 */
void set_render_pos(int x, int y) noexcept;

/// get render position
auto get_render_pos() noexcept -> glm::vec2;

// help macro, use for tmporary render in specific position
#define Tmp_Render_Pos(__x, __y) \
  for (auto __call_once = true; __call_once;) \
    for (auto __old_render_pos = get_render_pos(); __call_once; set_render_pos(__old_render_pos.x, __old_render_pos.y)) \
      for (set_render_pos(__x, __y); __call_once; __call_once = false)

////////////////////////////////////////////////////////////////////////////////
///                            Geometry
////////////////////////////////////////////////////////////////////////////////

/**
 * draw a rectangle
 * @param left_top left upper corner
 * @param right_bottom right down corner
 * @param color
 * @param thickness
 */
void rectangle(glm::vec2 left_top, glm::vec2 right_bottom, Color color = {}, float thickness = {}) noexcept;

/**
 * draw a triangle (clockwise)
 * @param p1
 * @param p2
 * @param p3
 * @param color
 * @param thickness
 */
void triangle(glm::vec2 p0, glm::vec2 p1, glm::vec2 p2, Color color = {}, float thickness = {}) noexcept;

/**
 * draw a circle
 * @param center
 * @param radius
 * @param color
 * @param thickness
 */
void circle(glm::vec2 center, float radius, Color color = {}, float thickness = {}) noexcept;

/**
 * draw a line
 * @param p0
 * @param p1
 * @param color
 */
void line(glm::vec2 p0, glm::vec2 p1, Color color = {}) noexcept;

/**
 * draw a quadratic bezier
 * @param p0
 * @param p1
 * @param p2
 * @param color
 */
void bezier(glm::vec2 p0, glm::vec2 p1, glm::vec2 p2, Color color = 0) noexcept;

////////////////////////////////////////////////////////////////////////////////
///                               Widget
////////////////////////////////////////////////////////////////////////////////

/**
 * whether cursor hover on specific region
 * @param left_top
 * @param right_bottom
 */
auto is_hover_on(glm::vec2 left_top, glm::vec2 right_bottom) noexcept -> bool;

/**
 * whether cursor hover on specific region, disable mouse penetration
 * @param name
 * @param left_top
 * @param right_bottom
 */
auto is_hover_on(std::string_view name, glm::vec2 left_top, glm::vec2 right_bottom) noexcept -> bool;

/**
 * whether cursor click on specific region
 * @param left_top
 * @param right_bottom
 */
auto is_click_on(glm::vec2 left_top, glm::vec2 right_bottom) noexcept -> bool;

/**
 * draw a button, can draw an icon in the center of button
 * default have a color lerp animation when cursor hover on button and leave on it
 * TODO: add bitmap draw replace draw icon by hand
 * @param name name cannot be duplicate in the window
 * @param x
 * @param y
 * @param width
 * @param height
 * @param button_color
 * @param button_hover_color
 * @param mouse_down_color
 * @param icon_update_func the function be called for draw icon by ui draw api
 * @param icon_width
 * @param icon_height
 * @param icon_color
 * @param icon_hover_color
 */
auto button(
  std::string_view                        name,
  int                                     x,
  int                                     y,
  uint32_t                                width,
  uint32_t                                height,
  Color                                   button_color,
  Color                                   button_hover_color,
  std::optional<Color>                    mouse_down_color = {},
  std::function<void(uint32_t, uint32_t)> icon_update_func = {},
  uint32_t                                icon_width       = {},
  uint32_t                                icon_height      = {},
  Color                                   icon_color       = {},
  Color                                   icon_hover_color = {}) noexcept-> bool;

}}
