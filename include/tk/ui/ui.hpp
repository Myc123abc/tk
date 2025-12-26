#pragma once

#include <string_view>

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

// render windows
void render() noexcept;

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
 */
void begin(std::string_view name, int x, int y, uint32_t width, uint32_t height, bool* is_closed) noexcept;

// end a window
void end() noexcept;

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

}}
