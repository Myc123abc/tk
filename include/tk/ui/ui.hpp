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
auto lerp(Color x, Color y, float v) noexcept -> glm::vec4;

/**
 * get lerp value of two points
 * @param x
 * @param y
 * @param v
 * @return lerp value
 */
auto lerp(glm::vec2 x, glm::vec2 y, float v) noexcept -> glm::vec2;

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

/**
 * get a lerp value in ping pong
 * @param name unique global name
 * @param b drive boolean value
 * @param duration duration (us)
 * @return lerp value (0.0 ~ 1.0)
 */
auto lerp_ping_pong(std::string_view name, bool b, double duration) noexcept -> double;

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

/**
 * set cursor move invalid area
 * every frame rendering will be reset
 * @param x
 * @param y
 * @param width
 * @param height
 */
void add_move_invalid_area(glm::vec2 left_top, glm::vec2 right_bottom) noexcept;

/**
 * get window extent in current update function of window
 * @return extent of window
 */
auto window_extent() noexcept -> glm::vec2;

/**
 * get window extent in current update function of window
 * @return extent of window without titlebar
 */
auto window_drawable_extent() noexcept -> glm::vec2;

////////////////////////////////////////////////////////////////////////////////
///                            Shape Operator
////////////////////////////////////////////////////////////////////////////////

/**
 * discard the pixel of specific rectangle for last draw shape
 * @param left_top
 * @param right_bottom
 */
void discard_rectangle(glm::vec2 left_top, glm::vec2 right_bottom) noexcept;

/// use path draw between lines and beziers
void begin_path() noexcept;

/**
 * end the path draw
 * @param color
 * @param thickness
 */
void end_path(Color color = {}, float thickness = {}) noexcept;

/// use union operator between shapes
void begin_union() noexcept;

/**
 * end the union operator
 * @param color
 * @param thickness
 */
void end_union(Color color, float thickness = {}) noexcept;

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
 * normal button
 * @param name name cannot be duplicate in the window
 * @param x
 * @param y
 * @param width
 * @param height
 * @param button_color
 * @param button_hover_color
 */
auto button(
  std::string_view name,
  int              x,
  int              y,
  uint32_t         width,
  uint32_t         height,
  Color            button_color,
  Color            button_hover_color) noexcept-> bool;

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
  std::optional<Color>                    mouse_down_color,
  std::function<void(uint32_t, uint32_t)> icon_update_func,
  uint32_t                                icon_width,
  uint32_t                                icon_height,
  Color                                   icon_color,
  Color                                   icon_hover_color) noexcept-> bool;

}}
