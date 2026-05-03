#pragma once

#include "base.hpp"

namespace tk {

struct Rect
{
  union
  {
    struct
    {
      float left{};
      float top{};
      float right{};
      float bottom{};
    };
    float4 data;
  };

  Rect() = default;
  Rect(float2 left_top, float2 right_bottom) noexcept : data(left_top, right_bottom) {}
  Rect(float left, float top, float right, float bottom) noexcept : left(left), top(top), right(right), bottom(bottom) {}
};

}
