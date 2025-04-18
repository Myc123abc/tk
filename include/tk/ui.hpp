//
// UI library
//
// implement by graphics engine
//
// HACK:
// 1. layout managed by ui, but button managed by user
//

#pragma once

#include "Window.hpp"
#include "GraphicsEngine/GraphicsEngine.hpp"
#include "type.hpp"

#include <glm/gtc/matrix_transform.hpp>

#include <vector>
#include <memory>

namespace tk {

  class ui
  {
  public:
    static void init(graphics_engine::GraphicsEngine* engine);

    struct UIWidget;
    struct Layout
    {
      tk::Window*            window = nullptr;
      uint32_t               x = 0;
      uint32_t               y = 0;
      std::vector<UIWidget*> widgets;
    };

    class UIWidget
    {
    public:
      UIWidget() = default;
      UIWidget(ShapeType type)
        : type(type) {}
      UIWidget(ShapeType type, Color color)
        : type(type), _color(color) {}
      virtual ~UIWidget() = default;
      UIWidget& operator=(UIWidget const& uw)
      {
        _layout = uw._layout;
        _x      = uw._x;
        _y      = uw._y;
        return *this;
      }

      auto set_layout(Layout* layout)           -> UIWidget&
      {
        _layout = layout;
        return *this;
      }

      auto set_position(uint32_t x, uint32_t y) -> UIWidget&
      {
        _x = x;
        _y = y;
        return *this;
      }

      auto set_color(Color color)               -> UIWidget&
      {
        _color = color;
        return *this;
      }

      auto get_color() { return _color; }

    public:
      const ShapeType type = ShapeType::Unknow;

    protected:
      Layout*  _layout = nullptr;
      uint32_t _x      = 0;
      uint32_t _y      = 0;
      Color    _color;
    };

    class Button : public UIWidget
    {
    public:
      Button() : UIWidget(ShapeType::Quard) {}
      Button(uint32_t width, uint32_t height, Color color)
        : UIWidget(ShapeType::Quard, color),
          _width(width), _height(height) {}
      ~Button() = default;
      Button& operator=(Button const& btn)
      {
        UIWidget::operator=(btn);
        _width  = btn._width;
        _height = btn._height;
        return *this;
      }

      bool is_clicked() { return false; }

      auto make_model_matrix() -> glm::mat4
      {
        assert(_layout && _width > 0 && _height > 0);
        uint32_t window_width, window_height;
        _layout->window->get_framebuffer_size(window_width, window_height);
        auto scale_x = (float)_width / window_width;
        auto scale_y = (float)_height / window_height;
        auto translate_x = (_layout->x + (float)_width  / 2 + _x) / ((float)window_width  / 2) - 1.f;
        auto translate_y = (_layout->y + (float)_height / 2 + _y) / ((float)window_height / 2) - 1.f;
        auto model = glm::mat4(1.f);
        model = glm::translate(model, glm::vec3(translate_x, translate_y, 0.f));
        model = glm::scale(model, glm::vec3(scale_x, scale_y, 1.f));
        return model;
      }

      void set_width_height(uint32_t width, uint32_t height) 
      {
        _width = width;
        _height = height;
      }

    private:
      uint32_t _width = 0, _height = 0;
    };

    static auto create_layout() -> Layout*;
    static auto create_button(uint32_t width, uint32_t height, Color color) -> Button*;

    static void put(Layout* layout, tk::Window* window, uint32_t x, uint32_t y);
    static void put(UIWidget* widget, Layout* layout, uint32_t x, uint32_t y);

    // HACK: use _layouts update matrix every frame, maybe performance suck 
    static void render();

  private:
    ui()                     = delete;
    ~ui()                    = delete;
    ui(ui const&)            = delete;
    ui(ui&&)                 = delete;
    ui& operator=(ui const&) = delete;
    ui& operator=(ui&&)      = delete;

    inline static graphics_engine::GraphicsEngine*       _engine;
    inline static std::vector<std::unique_ptr<Layout>>   _layouts;
    inline static std::vector<std::unique_ptr<UIWidget>> _widgets;
  };

}
