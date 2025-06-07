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
    
    path,
    line_partition,
    bezier_partition,
  };

  enum class shape_op
  {
    mix,
    min,
  };

}}