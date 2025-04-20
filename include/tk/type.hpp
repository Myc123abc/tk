//
// common type library
//

#pragma once

namespace tk
{

  enum class ShapeType
  {
    Unknow,
    Quard,
    Circle,
  };

  enum class Color
  {
    Unknow,
    Red,
    Green,
    Blue,
    Yellow,
    OneDark,
  };

  enum class UIType
  {
    UIWidget        = 0b00,
    ClickableWidget = 0b01,
    Button          = 0b11,
  };

}
