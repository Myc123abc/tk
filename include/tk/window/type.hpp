#pragma once

#include <type_traits>

namespace tk { namespace window {

enum class MouseState
{
  idle                  = 0b0000,
  left_button_down      = 0b0001,
  left_button_down_idle = 0b0011,
  left_button_press     = 0b0100,
  left_button_up        = 0b1000,
};

inline constexpr auto operator&(MouseState lhs, MouseState rhs) noexcept
{
  using T = std::underlying_type_t<MouseState>;
  return static_cast<T>(lhs) & static_cast<T>(rhs);
}

}}
