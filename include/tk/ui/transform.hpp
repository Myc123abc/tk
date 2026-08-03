#pragma once

#include "tk/base.hpp"

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
};

class Transform
{
public:
  Transform()                            = default;
  ~Transform()                           = default;
  Transform(Transform const&)            = delete;
  Transform(Transform&&)                 = delete;
  Transform& operator=(Transform const&) = delete;
  Transform& operator=(Transform&&)      = delete;

  auto translate(float x, float y) noexcept -> Transform&
  {
    _m *= { 1, 0, 0, 1, x, y };
    return *this;
  }

  auto rotate(float angle) noexcept -> Transform&
  {
    auto c = std::cos(angle);
    auto s = std::sin(angle);
    _m *= { c, s, -s, c, 0, 0 };
    return *this;
  }

  auto rotate(float2 p, float angle) noexcept -> Transform&
  {
    return translate(-p.x, -p.y)
          .rotate(angle)
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

  auto matrix() const noexcept -> Matrix const& { return _m; }

private:
  Matrix _m;
};

}
