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

#include "../tk.hpp"
#include "../GraphicsEngine/Painter.hpp"

namespace tk { namespace ui {

  class UI
  {
  public:

    /**
     * initialize UI
     * @param window main window
     */
    static void init(tk_context& ctx);
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
    inline static tk_context*              _ctx;
    inline static graphics_engine::Painter _painter;
    inline static uint32_t                 _background_id = -1;
  };

}}
