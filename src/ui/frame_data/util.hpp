#pragma once

#include "util/base.hpp"

#include <span>

namespace tk::ui {

inline auto fast_rsqrt(float x) noexcept
{
  return _mm_cvtss_f32(_mm_rsqrt_ss(_mm_set_ss(x)));
}

inline auto fast_normalize(float2 p) noexcept
{
  auto d2 = dot(p, p);
  if (d2 > .0f)
    return p * fast_rsqrt(d2);
  return float2{};
}

inline auto fix_normal(float2 p) noexcept
{
  auto d2 = dot(p, p);
  if (d2 > 0.000001f)
  {
    auto inv_len2 = 1.f / d2;
    auto const max = 100.f;
    if (inv_len2 > max)
      inv_len2 = max;
    return p * inv_len2;
  }
  return float2{};
}

inline auto round_up_to_even(int x) noexcept
{
  return ((x + 1) / 2) * 2;
}

inline auto calc_circle_radius(float cnt, float max_error) noexcept
{
  return max_error / (1 - std::cos(std::numbers::pi_v<float> / std::max(cnt, std::numbers::pi_v<float>)));
}

inline auto is_convex(std::span<float2> p) noexcept
{
  auto const n = p.size();
  if (n < 3)
    return false;

  auto sign = 0.0f;
  for (auto i = 0u; i < n; ++i)
  {
    auto const a = p[i];
    auto const b = p[(i + 1) % n];
    auto const c = p[(i + 2) % n];
    auto const v = cross(b - a, c - b);

    if (std::abs(v) < 1e-6f)
      continue;
    if (sign == .0f)
      sign = v;
    else if ((v > 0.0f) != (sign > 0.0f))
      return false;
  }

  return true;
}

inline auto angle_of(float2 center, float2 p) noexcept
{
  return std::atan2(p.y - center.y, p.x - center.x);
}

}
