#pragma once

#include "vec.hpp"

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
    vec4 data;
  };

  Rect() = default;
  Rect(vec2 left_top, vec2 right_bottom) noexcept : data(left_top, right_bottom) {}
  Rect(float left, float top, float right, float bottom) noexcept : left(left), top(top), right(right), bottom(bottom) {}
};

}
