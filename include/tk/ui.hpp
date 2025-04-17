//
// UI library
//
// implement by graphics engine
//
// HACK:
// 1. how to use graphics engine with other class in future?
// 2. engine.resize_swapchain how to elegent handle 
//

#pragma once

#include "Window.hpp"

#include <cinttypes>

namespace tk { namespace ui {

  struct Layout
  {

  };

  class UIWidget
  {
  public:
    virtual ~UIWidget() = default;
  };

  class Button : public UIWidget
  {
  public:
    ~ Button() = default;
    bool is_clicked() { return false; }
  };

  auto create_layout() -> Layout;
  auto create_button(uint32_t width, uint32_t height) -> Button;

  void put(Layout& layout, tk::Window& window, uint32_t x, uint32_t y);
  void put(UIWidget& widget, Layout& layout, uint32_t x, uint32_t y);

  void render();

  class UI
  {
  private:
    UI()  = delete;
    ~UI() = delete;

    UI(UI const&)            = delete;
    UI(UI&&)                 = delete;
    UI& operator=(UI const&) = delete;
    UI& operator=(UI&&)      = delete;

  private:
  };

}}
