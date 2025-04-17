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
#include "GraphicsEngine/GraphicsEngine.hpp"

namespace tk {

  class ui
  {
  private:
    ui()                     = delete;
    ~ui()                    = delete;
    ui(ui const&)            = delete;
    ui(ui&&)                 = delete;
    ui& operator=(ui const&) = delete;
    ui& operator=(ui&&)      = delete;

    inline static graphics_engine::GraphicsEngine* _engine;

  public:
    static auto init(graphics_engine::GraphicsEngine* engine) { _engine = engine; }

    struct UIWidget;
    struct UIWidgetInfo
    {
      UIWidget* widget;
      uint32_t  x, y;
    };

    struct Layout
    {
      tk::Window*               window;
      uint32_t                  x, y; 
      std::vector<UIWidgetInfo> widget_infos;
    };

    class UIWidget
    {
    public:
      virtual ~UIWidget() = default;
    };

    class Button : public UIWidget
    {
    public:
      Button() = default;
      Button(uint32_t width, uint32_t height)
        : _width(width), _height(height) {}
      ~Button() = default;
      bool is_clicked() { return false; }

    private:
      uint32_t _width, _height;
    };

    static auto create_layout() -> Layout;
    static auto create_button(uint32_t width, uint32_t height) -> Button;

    static void put(Layout& layout, tk::Window& window, uint32_t x, uint32_t y);
    static void put(UIWidget& widget, Layout& layout, uint32_t x, uint32_t y);

    static void render();
  };

}
