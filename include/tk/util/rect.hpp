#pragma once

#include "base.hpp"
#include "mixins/replaceable.hpp"

#include <windows.h>

#include <span>
#include <cfloat>

namespace tk {

struct Rect : Replaceable
{
  float left{ FLT_MAX };
  float top{ FLT_MAX };
  float right{ -FLT_MAX };
  float bottom{ -FLT_MAX };

  Rect() = default;

  static auto zero() noexcept { return Rect{ 0, 0, 0, 0 }; }

  auto operator=(Rect const&) noexcept -> Rect& = default;

  auto empty() const noexcept { return left >= right || top >= bottom; }

  template <Vec2 LTT, Vec2 RBT>
  Rect(LTT left_top, RBT right_bottom) noexcept :
    left(static_cast<float>(left_top.x)),
    top(static_cast<float>(left_top.y)),
    right(static_cast<float>(right_bottom.x)),
    bottom(static_cast<float>(right_bottom.y)) {}

  Rect(RECT rc) noexcept : left(rc.left), top(rc.top), right(rc.right), bottom(rc.bottom) {}
  Rect(HWND hwnd) noexcept
  {
    auto rc = RECT{};
    GetWindowRect(hwnd, &rc);
    left   = rc.left;
    top    = rc.top;
    right  = rc.right;
    bottom = rc.bottom;
  }

  template <Numeric X, Numeric Y, Vec2 Ext>
  Rect(X x, Y y, Ext ext) : left(x), top(y), right(x + ext.x), bottom(y + ext.y) {}

  template <Numeric LT, Numeric TT, Numeric RT, Numeric BT>
  Rect(LT left, TT top, RT right, BT bottom) noexcept :
    left(static_cast<float>(left)),
    top(static_cast<float>(top)),
    right(static_cast<float>(right)),
    bottom(static_cast<float>(bottom)) {}

  auto operator==(Rect rc) const noexcept { return left == rc.left && top == rc.top && right == rc.right && bottom == rc.bottom; }
  auto operator!=(Rect rc) const noexcept { return !operator==(rc); }

  template <Vec2 T>
  auto operator+(T offset) const noexcept
  {
    auto rc = *this;
    rc.left   += offset.x;
    rc.top    += offset.y;
    rc.right  += offset.x;
    rc.bottom += offset.y;
    return rc;
  }

  template <Vec2 T>
  auto& operator+=(T offset) noexcept { left += offset.x; top += offset.y; right += offset.x; bottom += offset.y; return *this; }

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

  auto width()  const noexcept { return right  - left;               }
  auto height() const noexcept { return bottom - top;                }
  auto pos()    const noexcept { return float2{ left, top };         }
  auto extent() const noexcept { return float2{ width(), height() }; }
};

inline auto intersect(Rect lhs, Rect rhs) noexcept
{
  return Rect
  {
    std::max(lhs.left,   rhs.left),
    std::max(lhs.top,    rhs.top),
    std::min(lhs.right,  rhs.right),
    std::min(lhs.bottom, rhs.bottom),
  };
}

}
