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

  enum class mouse
  {
    left_down,
    left_up,
  };

  enum class window
  {
    running,
    closed,
    suspended,
  };

  enum class key
  {
    space,
    q,
  };

  enum class key_state
  {
    press,
    repeate_wait,
    release,
  };

}}