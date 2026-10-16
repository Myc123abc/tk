#pragma once

#include "tk/rect.hpp"

#include <algorithm>
#include <cmath>
#include <optional>

namespace tk::ui {

struct Matrix
{
  float m11{ 1 }, m12{};
  float m21{}, m22{ 1 };
  float tx{}, ty{};

  static auto ScaleTranslate(float uniform_scale, float2 offset) noexcept -> Matrix
  {
    return
    {
      .m11 = uniform_scale,
      .m22 = uniform_scale,
      .tx  = offset.x,
      .ty  = offset.y,
    };
  }

  auto operator*(float2 p) const noexcept -> float2
  {
    return { p.x * m11 + p.y * m21 + tx, p.x * m12 + p.y * m22 + ty };
  }

  auto transform_bounds(Rect rect) const noexcept -> Rect
  {
    auto result = Rect{};
    result.expand(*this * rect.left_top());
    result.expand(*this * float2{ rect.right, rect.top });
    result.expand(*this * rect.right_bottom());
    result.expand(*this * float2{ rect.left, rect.bottom });
    return result;
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

  auto inverse() const noexcept -> std::optional<Matrix>
  {
    auto const determinant = m11 * m22 - m12 * m21;
    if (!std::isfinite(determinant) || determinant == 0)
      return {};

    auto result = Matrix
    {
       m22 / determinant,
      -m12 / determinant,
      -m21 / determinant,
       m11 / determinant,
       0,
       0,
    };
    result.tx = -(tx * result.m11 + ty * result.m21);
    result.ty = -(tx * result.m12 + ty * result.m22);
    return result;
  }

  auto uniform_scale() const noexcept -> std::optional<float>
  {
    auto const x_len_sq = m11 * m11 + m12 * m12;
    auto const y_len_sq = m21 * m21 + m22 * m22;
    auto const axes_dot = m11 * m21 + m12 * m22;
    auto const tolerance = std::max(x_len_sq, y_len_sq) * 0.0001f + 0.000001f;
    auto const determinant = m11 * m22 - m12 * m21;

    if (determinant <= 0 ||
        std::abs(x_len_sq - y_len_sq) > tolerance ||
        std::abs(axes_dot) > tolerance)
      return {};

    return std::sqrt((x_len_sq + y_len_sq) * .5f);
  }

  auto axis_aligned_uniform_scale() const noexcept -> std::optional<float>
  {
    auto scale = uniform_scale();
    if (!scale) return {};

    auto const tolerance = scale.value() * 0.0001f + 0.000001f;
    if (m11 <= 0 || m22 <= 0 ||
        std::abs(m12) > tolerance ||
        std::abs(m21) > tolerance)
      return {};

    return scale;
  }

  auto scale_bound() const noexcept
  {
    return std::sqrt(m11 * m11 + m12 * m12 + m21 * m21 + m22 * m22);
  }

  auto empty() const noexcept
  {
    return m11 == 1 && m12 == 0 && m21 == 0 && m22 == 1 && tx == 0 && ty == 0;
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

  auto translate(float2 offset) noexcept -> Transform&
  {
    _m *= { 1, 0, 0, 1, offset.x, offset.y };
    return *this;
  }

  static auto Translate(float x, float y) noexcept -> Transform
  {
    return Transform{}.translate(x, y);
  }

  static auto Translate(float2 offset) noexcept -> Transform
  {
    return Transform{}.translate(offset);
  }

  auto rotate(float degrees) noexcept -> Transform&
  {
    auto angle = static_cast<float>(radians(degrees));
    auto c    = std::cos(angle);
    auto s    = std::sin(angle);
    _m *= { c, s, -s, c, 0, 0 };
    return *this;
  }

  static auto Rotate(float degrees) noexcept -> Transform
  {
    return Transform{}.rotate(degrees);
  }

  auto rotate(float2 p, float degrees) noexcept -> Transform&
  {
    return translate(-p.x, -p.y)
          .rotate(degrees)
          .translate(p.x, p.y);
  }

  static auto Rotate(float2 p, float degrees) noexcept -> Transform
  {
    return Transform{}.rotate(p, degrees);
  }

  auto scale(float sx, float sy) noexcept -> Transform&
  {
    _m *= { sx, 0, 0, sy, 0, 0 };
    return *this;
  }

  static auto Scale(float sx, float sy) noexcept -> Transform
  {
    return Transform{}.scale(sx, sy);
  }

  auto operator()(float2 p) const noexcept -> float2
  {
    return _m * p;
  }

  auto operator*(float2 p) const noexcept -> float2
  {
    return _m * p;
  }

  auto matrix() const noexcept -> Matrix const& { return _m; }

  auto empty() const noexcept { return _m.empty(); }

private:
  Matrix _m;
};

}
