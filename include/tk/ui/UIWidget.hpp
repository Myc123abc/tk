//
// UI Widgets
//

#pragma once

#include "../type.hpp"
#include "../Window.hpp"

#include <glm/gtc/matrix_transform.hpp>

#include <vector>

namespace tk { namespace ui {

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
    virtual ~UIWidget() = default;

    UIWidget(ShapeType type)
      : type(type) {}
    UIWidget(ShapeType type, Color color)
      : type(type), _color(color) {}

    UIWidget& operator=(UIWidget const& uw)
    {
      _layout = uw._layout;
      _x      = uw._x;
      _y      = uw._y;
      return *this;
    }

    auto set_layout(Layout* layout)           -> UIWidget&;
    auto set_position(uint32_t x, uint32_t y) -> UIWidget&;
    auto set_color(Color color)               -> UIWidget&;
    auto set_depth(float depth)               -> UIWidget&;

    auto get_color() { return _color; }
    auto get_depth() { return _depth; }

  public:
    const ShapeType type = ShapeType::Unknow;

  protected:
    Layout*  _layout = nullptr;
    uint32_t _x      = 0;
    uint32_t _y      = 0;
    Color    _color;
    float    _depth  = 0.f;
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

    bool is_clicked(); 

    auto make_model_matrix() -> glm::mat4;

    void set_width_height(uint32_t width, uint32_t height);
    
  private:
    uint32_t _width = 0, _height = 0;
  };

}}
