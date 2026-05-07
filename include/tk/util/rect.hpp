#pragma once

#include "base.hpp"

#include <windows.h>

#include <span>
#include <cfloat>

namespace tk {

struct Rect
{
  float left{ FLT_MAX };
  float top{ FLT_MAX };
  float right{ -FLT_MAX };
  float bottom{ -FLT_MAX };

  Rect() = default;

  auto empty() const noexcept { return left >= right || top >= bottom; }

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

  void expand(float2 p) noexcept
  {
    left   = fmin(p.x, left);
    top    = fmin(p.y, top);
    right  = fmax(p.x, right);
    bottom = fmax(p.y, bottom);
  }

  void expand(std::span<float2> pts) noexcept
  {
    for (auto const& pt : pts)
    {
      left   = fmin(pt.x, left);
      top    = fmin(pt.y, top);
      right  = fmax(pt.x, right);
      bottom = fmax(pt.y, bottom);
    }
  }

  Rect(std::span<float2> pts) noexcept { expand(pts); }
};

}
