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

    auto set_shape_type(ShapeType shape_type) -> UIWidget&;
    auto set_layout(Layout* layout)           -> UIWidget&;
    auto set_position(uint32_t x, uint32_t y) -> UIWidget&;
    auto set_color(glm::vec3 const& color)    -> UIWidget&;
    auto set_depth(float depth)               -> UIWidget&;

    auto set_shape_properties(std::initializer_list<uint32_t> values) -> UIWidget&;

    auto get_type()       const noexcept { return _type; }
    auto get_shape_type() const noexcept { return _shape_type; }
    auto get_color()      const noexcept { return _color; }
    auto get_depth()      const noexcept { return _depth; }

    void check_property_values();

    void remove_from_layout()
    {
      _layout = nullptr;
      _x      = 0;
      _y      = 0;
    }

    auto generate_model_matrix()        -> glm::mat4;

  private:
    auto generate_quard_model_matrix()  -> glm::mat4;
    auto generate_circle_model_matrix() -> glm::mat4;

  protected:
    UIType    _type       = UIType::UIWidget;
    ShapeType _shape_type = ShapeType::Unknow;
    Layout*   _layout     = nullptr;
    uint32_t  _x          = 0;
    uint32_t  _y          = 0;
    glm::vec3 _color;
    // INFO: default depth value is .1f, is to convience set background depth is 0.f
    float     _depth      = .1f;

    std::vector<uint32_t> _property_values;
  };

  class ClickableWidget : public UIWidget
  {
  public:
    ClickableWidget() { _type = UIType::ClickableWidget; }
    virtual ~ClickableWidget() = default;

    bool is_mouse_over();

    void set_is_clicked() noexcept { _is_clicked = true; }
    bool is_clicked()     noexcept
    {
      auto res = _is_clicked;
      if (res)
      {
        _is_clicked = false;
      }
      return res;
    }

  private:
    bool mouse_over_quard();
    bool mouse_over_circle();

  private:
    bool _is_clicked  = false;
  };

  class Button: public ClickableWidget
  {
  public:
    Button() { _type = UIType::Button; }
  };

}}
