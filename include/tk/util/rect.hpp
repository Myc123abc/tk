#pragma once

#include "base.hpp"

#include <windows.h>

namespace tk {

struct Rect
{
  LONG left;
  LONG top;
  LONG right;
  LONG bottom;

  Rect() = default;

  template <Vec2 LTT, Vec2 RBT>
  Rect(LTT left_top, RBT right_bottom) noexcept :
    left(static_cast<LONG>(left_top.x)),
    top(static_cast<LONG>(left_top.y)),
    right(static_cast<LONG>(right_bottom.x)),
    bottom(static_cast<LONG>(right_bottom.y)) {}

  Rect(RECT rc) noexcept : left(rc.left), top(rc.top), right(rc.right), bottom(rc.bottom) {}

  template <Numeric LT, Numeric TT, Numeric RT, Numeric BT>
  Rect(LT left, TT top, RT right, BT bottom) noexcept :
    left(static_cast<LONG>(left)),
    top(static_cast<LONG>(top)),
    right(static_cast<LONG>(right)),
    bottom(static_cast<LONG>(bottom)) {}

  auto operator==(Rect rc) const noexcept { return left == rc.left && top == rc.top && right == rc.right && bottom == rc.bottom; }
  auto operator!=(Rect rc) const noexcept { return !operator==(rc); }

  auto RECT_ptr() noexcept { return reinterpret_cast<RECT*>(this); }
  auto& RECT_ref() noexcept { return *RECT_ptr(); }

  template <Vec2 T>
  auto contains(T p) const noexcept { return p.x > left && p.x < right && p.y > top && p.y < bottom; }

  template <Vec2 T>
  auto contains_bounding(T p) const noexcept { return p.x >= left && p.x <= right && p.y >= top && p.y <= bottom; }
};

}
