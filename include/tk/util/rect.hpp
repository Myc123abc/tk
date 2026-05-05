#pragma once

#include "base.hpp"

#include <windows.h>

namespace tk {

struct Rect
{
  float left;
  float top;
  float right;
  float bottom;

  Rect() = default;

  template <Vec2 LTT, Vec2 RBT>
  Rect(LTT left_top, RBT right_bottom) noexcept :
    left(static_cast<float>(left_top.x)),
    top(static_cast<float>(left_top.y)),
    right(static_cast<float>(right_bottom.x)),
    bottom(static_cast<float>(right_bottom.y)) {}

  Rect(RECT rc) noexcept : left(rc.left), top(rc.top), right(rc.right), bottom(rc.bottom) {}

  template <Numeric LT, Numeric TT, Numeric RT, Numeric BT>
  Rect(LT left, TT top, RT right, BT bottom) noexcept :
    left(static_cast<float>(left)),
    top(static_cast<float>(top)),
    right(static_cast<float>(right)),
    bottom(static_cast<float>(bottom)) {}

  auto operator==(Rect rc) const noexcept { return left == rc.left && top == rc.top && right == rc.right && bottom == rc.bottom; }
  auto operator!=(Rect rc) const noexcept { return !operator==(rc); }

  template <Vec2 T>
  auto contains(T p) const noexcept { return p.x > left && p.x < right && p.y > top && p.y < bottom; }

  template <Vec2 T>
  auto contains_bounding(T p) const noexcept { return p.x >= left && p.x <= right && p.y >= top && p.y <= bottom; }

  auto to_RECT() const noexcept { return RECT{ static_cast<LONG>(left), static_cast<LONG>(top), static_cast<LONG>(right), static_cast<LONG>(bottom) }; }
};

}
