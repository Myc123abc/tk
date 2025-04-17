//
// tk context
//
// contain all global resources
//
// TODO:
// 1. all static resources create in here
//

#pragma once

#include "Window.hpp"
#include "GraphicsEngine/GraphicsEngine.hpp"

namespace tk
{
  
  struct tk_context
  {
    Window                          window;
    graphics_engine::GraphicsEngine engine;

    /**
     * initialize tk context
     * create main window
     * initialize graphics engine
     * @param title title of main window
     * @param width width of main window
     * @param height height of main window
     */
    void init(std::string_view title, uint32_t width, uint32_t height)
    {
      window.init(title, width, height);
      engine.init(window);
    }

    void destroy()
    {
      engine.destroy();
      window.destroy();
    }
  };

}
