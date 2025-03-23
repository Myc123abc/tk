//
// graphics engine
//
// use Vulkan to implement
//

#pragma once

#include "Window.hpp"

namespace tk
{
  class GraphicsEngine final
  {
  public:
    GraphicsEngine(const Window& window);
    ~GraphicsEngine();

    GraphicsEngine(const GraphicsEngine&)            = delete;
    GraphicsEngine(GraphicsEngine&&)                 = delete;
    GraphicsEngine& operator=(const GraphicsEngine&) = delete;
    GraphicsEngine& operator=(GraphicsEngine&&)      = delete;

  private:
    //
    // initialize resources
    //
    void create_instance();

  private:
    // HACK: expand to multi-windows manage, use WindowManager in future.
    const Window& _window;

    VkInstance    _instance;
  };
}
