#pragma once

namespace tk { namespace type {

  enum class Shape
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

    glyph,
  };

  enum class ShapeOp 
  {
    mix,
    min,
  };

  enum class MouseState
  {
    left_down,
    left_up,
  };

  enum class WindowState
  {
    running,
    closed,
    suspended,
  };

  enum class Key
  {
    space,
    q,
  };

  enum class KeyState
  {
    press,
    repeate_wait,
    release,
  };

}}