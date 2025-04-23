//
// line
//
// HACK: this line not have click property
//

#pragma once

#include "UIWidget.hpp"

namespace tk { namespace ui {

  class Line : public UIWidget 
  {
  public:
    Line() 
      : UIWidget(ShapeType::Quard) {}

    auto& set_length(uint32_t length) noexcept
    {
      _property_values[0] = length;
      return *this;
    }

    auto& set_width(uint32_t width) noexcept
    {
      _property_values[1] = width;
      return *this;
    }
  };

}}
