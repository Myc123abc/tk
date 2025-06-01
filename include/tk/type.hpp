#pragma once

namespace tk { namespace type {

  enum class shape
  {
    line,
    rectangle,
    triangle,
    polygon,
    circle,
    bezier,
  };

  enum class shape_op
  {
    mix,
    min,
  };

}}