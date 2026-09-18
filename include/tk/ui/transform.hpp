#pragma once

#include "tk/base.hpp"
#include "tk/rect.hpp"

namespace tk::ui {

struct Matrix
{
  float m11{ 1 }, m12{};
  float m21{}, m22{ 1 };
  float tx{}, ty{};

  auto operator*(float2 p) const noexcept -> float2
  {
    return { p.x * m11 + p.y * m21 + tx, p.x * m12 + p.y * m22 + ty };
  }

  auto operator*(Matrix const& m) const noexcept -> Matrix
  {
    return {
      m11 * m.m11 + m12 * m.m21,
      m11 * m.m12 + m12 * m.m22,
      m21 * m.m11 + m22 * m.m21,
      m21 * m.m12 + m22 * m.m22,
      tx  * m.m11 + ty  * m.m21 + m.tx,
      tx  * m.m12 + ty  * m.m22 + m.ty,
    };
  }

  auto operator*=(Matrix const& m) noexcept -> Matrix&
  {
    (*this) = (*this) * m;
    return *this;
  }

  auto transform_rect(float2 pos, float2 extent) const noexcept -> Rect
  {
    auto rect = Rect{};
    rect.expand((*this) * pos);
    rect.expand((*this) * float2{ pos.x + extent.x, pos.y });
    rect.expand((*this) * (pos + extent));
    rect.expand((*this) * float2{ pos.x, pos.y + extent.y });
    return rect;
  }

  auto transform_extent(float2 extent) const noexcept -> float2
  {
    return transform_rect({}, extent).extent();
  }
};

class Transform
{
public:
  auto translate(float x, float y) noexcept -> Transform&
  {
    _m *= { 1, 0, 0, 1, x, y };
    return *this;
  }

  auto rotate(float degrees) noexcept -> Transform&
  {
    auto angle = static_cast<float>(radians(degrees));
    auto c    = std::cos(angle);
    auto s    = std::sin(angle);
    _m *= { c, s, -s, c, 0, 0 };
    return *this;
  }

  auto rotate(float2 p, float degrees) noexcept -> Transform&
  {
    return translate(-p.x, -p.y)
          .rotate(degrees)
          .translate(p.x, p.y);
  }

  auto scale(float sx, float sy) noexcept -> Transform&
  {
    _m *= { sx, 0, 0, sy, 0, 0 };
    return *this;
  }

  auto operator()(float2 p) const noexcept -> float2
  {
    return _m * p;
  }

  auto operator*(float2 p) const noexcept -> float2
  {
    return _m * p;
  }

  auto transform_rect(float2 pos, float2 extent) const noexcept -> Rect
  {
    return _m.transform_rect(pos, extent);
  }

  auto transform_extent(float2 extent) const noexcept -> float2
  {
    return _m.transform_extent(extent);
  }

  auto matrix() const noexcept -> Matrix const& { return _m; }

private:
  Matrix _m;
};

}
