#pragma once

#include "../util/base.hpp"

namespace tk::ui {

struct Color
{
  Color() = default;

  inline static constexpr auto inv_255 = 1.f / 255;

  Color(uint color) noexcept
  {
    r = static_cast<float>((color >> 24) & 0xFF) * inv_255;
    g = static_cast<float>((color >> 16) & 0xFF) * inv_255;
    b = static_cast<float>((color >> 8 ) & 0xFF) * inv_255;
    a = static_cast<float>((color      ) & 0xFF) * inv_255;
  }

  Color(float4 color) noexcept
    : r(color.x), g(color.y), b(color.z), a(color.w) {}

  Color(float r, float g, float b, float a) noexcept
    : r(r), g(g), b(b), a(a) {}

  operator float4() const noexcept { return { r, g, b, a }; }

  auto to_uint() const noexcept -> uint
  {
    auto to_u8 = [](float x) noexcept -> uint
    {
      x = std::clamp(x, 0.f, 1.f);
      return std::lround(x * 255);
    };
   return
      to_u8(r) << 24 |
      to_u8(g) << 16 |
      to_u8(b) <<  8 |
      to_u8(a);
  }

  auto operator==(Color col) const noexcept { return r == col.r && g == col.g && b == col.b && a == col.a; }
  auto operator!=(Color col) const noexcept { return !operator==(col); }

  float r{}, g{}, b{}, a{};
};

}
