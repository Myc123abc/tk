//
// layout
//

#pragma once

#include "../Window.hpp"

namespace tk { namespace ui {

  struct UIWidget;

  /**
   * layout contains multiple ui widgets can be rendered
   * it also be related with position of which window
   */
  class Layout
  {
  public:
    auto bind(Window const* window)           -> Layout&
    {
      _window = window;
      return *this;
    }

    auto set_position(uint32_t x, uint32_t y) -> Layout&
    {
      _x = x;
      _y = y;
      return *this;
    }

    auto& get_widgets() const noexcept { return _widgets; }

    auto push_widget(UIWidget* widget) -> Layout&
    {
      _widgets.push_back(widget);
      return *this;
    }

    auto get_window() const noexcept { return _window; }
    auto get_x()      const noexcept { return _x; }
    auto get_y()      const noexcept { return _y; }

  private:
    tk::Window const*      _window = nullptr;
    uint32_t               _x      = 0;
    uint32_t               _y      = 0;
    std::vector<UIWidget*> _widgets;
  };
  
}}
