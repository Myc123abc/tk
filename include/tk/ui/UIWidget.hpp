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

  /**
   * layout contains multiple ui widgets can be rendered
   * it also be related with position of which window
   */
  struct Layout
  {
    tk::Window*            window = nullptr;
    uint32_t               x = 0;
    uint32_t               y = 0;
    std::vector<UIWidget*> widgets;
  };

  /**
   * UIWidget base class
   * be related position of which layout
   * have shape type to specific use which shape mesh
   * and shape color
   * and depth
   */
  class UIWidget
  {
  public:
    virtual ~UIWidget() = default;

    auto set_type(ShapeType type)             -> UIWidget&;
    auto set_layout(Layout* layout)           -> UIWidget&;
    auto set_position(uint32_t x, uint32_t y) -> UIWidget&;
    auto set_color(Color color)               -> UIWidget&;
    auto set_depth(float depth)               -> UIWidget&;

    auto set_shape_properties(std::initializer_list<uint32_t> values) -> UIWidget&;

    auto get_type()  const noexcept { return _type;  }
    auto get_color() const noexcept { return _color; }
    auto get_depth() const noexcept { return _depth; }

    void check_property_values();

    // HACK: should be as interface class for expand
    virtual bool is_mouse_over() { return false; }
    void set_is_clicked() { _is_clicked = true; }
    bool is_clicked()
    {
      auto res = _is_clicked;
      if (res)
      {
        _is_clicked = false;
      }
      return res;
    }

    void remove_from_layout()
    {
      _layout = nullptr;
      _x      = 0;
      _y      = 0;
    }

  protected:
    ShapeType _type   = ShapeType::Unknow;
    Layout*   _layout = nullptr;
    uint32_t  _x      = 0;
    uint32_t  _y      = 0;
    Color     _color  = Color::Unknow;
    // INFO: default depth value is .1f, is to convience set background depth is 0.f
    float     _depth  = .1f;

    bool _is_clicked  = false;

    std::vector<uint32_t> _property_values;
  };

  /**
   * Button have width and height
   * and a clicked judget which is click down in button then release in button
   * and make model matrix from width and height with layout properties
   * HACK: button should inhert from Quard which have make_model_matrix
   *       and IClicked which have is_clicked
   */
  class Button : public UIWidget
  {
  public:
    bool is_mouse_over() override;

    auto make_model_matrix() -> glm::mat4;

  private:
  };

}}
