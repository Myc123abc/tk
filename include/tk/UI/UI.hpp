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
#include "GraphicsEngine.hpp"
#include "Painter.hpp"
#include "DestructorStack.hpp"

namespace tk { namespace ui {

  class UI
  {
  public:

    /**
     * initialize UI
     * default use OneDark as background
     * @param window main window
     */
    static void init(Window const& window);
    static void destroy();

    static void render();

  private:
    UI()  = delete;
    ~UI() = delete;

    UI(UI const&)            = delete;
    UI(UI&&)                 = delete;
    UI& operator=(UI const&) = delete;
    UI& operator=(UI&&)      = delete;

  private:
    inline static graphics_engine::GraphicsEngine _engine;
    inline static graphics_engine::Painter        _painter;
    inline static DestructorStack                 _destructor;

    inline static uint32_t _background_id = -1;
  };

}}
