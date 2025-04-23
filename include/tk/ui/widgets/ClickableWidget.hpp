//
// Clickable Widget
//
// add mouse handling
// can be use as Button
//

#pragma once

#include "UIWidget.hpp"

namespace tk { namespace ui {

  class ClickableWidget : public UIWidget
  {
  public:
    ClickableWidget(ShapeType shape_type) 
      : UIWidget(shape_type)
    {
      enable_clickable();
    }

    virtual ~ClickableWidget() = default;

    bool is_mouse_over();

    void set_is_clicked() noexcept { _is_clicked = true; }
    bool is_clicked()     noexcept
    {
      auto res = _is_clicked;
      if (res)
      {
        _is_clicked = false;
      }
      return res;
    }

  private:
    bool mouse_over_quard();
    bool mouse_over_circle();

  private:
    bool _is_clicked  = false;
  };


}}
