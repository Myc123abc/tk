//
// UI Widgets
//

#pragma once

#include "../../type.hpp"
#include "../Layout.hpp"
#include "../../ErrorHandling.hpp"

#include <glm/gtc/matrix_transform.hpp>

#include <vector>

namespace tk { namespace ui {

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
    UIWidget(ShapeType shape_type)
      : _shape_type(shape_type) {}

    virtual ~UIWidget() = default;

    // FIX: if one widget bind on multiple layout will be what?
    bool only_once = false;
    auto bind(Layout* layout)              -> UIWidget&
    {
      throw_if(only_once, "FIX: not consider bind multiple count");
      if (only_once == false) only_once = true;

      _layout = layout;
      _layout->push_widget(this);
      return *this;
    }

    auto set_position(uint32_t x, uint32_t y) -> UIWidget&
    {
      _x = x;
      _y = y;
      return *this;
    }

    auto set_color(glm::vec3 const& color)    -> UIWidget&
    {
      _color = color;
      return *this;
    }

    auto set_depth(float depth)               -> UIWidget&
    {
      throw_if(depth < 0.f || depth > 1.f, "depth should between 0.f and 1.f");
      _depth = depth;
      return *this;
    }

    auto set_rotation_angle(float angle)      -> UIWidget&
    {
      _rotation_angle = angle;
      return *this;
    }

    auto set_shape_properties(std::initializer_list<uint32_t> values) -> UIWidget&
    {
      _property_values = values;
      check_property_values();
      return *this;
    }

    auto get_shape_type() const noexcept { return _shape_type; }
    auto get_color()      const noexcept { return _color; }
    auto get_depth()      const noexcept { return _depth; }

    void remove_from_layout()
    {
      _layout = nullptr;
      _x      = 0;
      _y      = 0;
    }

    auto generate_model_matrix()        -> glm::mat4;

    //
    // features
    //
    auto clickable() const noexcept { return _clickable; }
  protected:
    auto enable_clickable() noexcept -> UIWidget&
    {
      _clickable = true;
      return *this;
    }

  private:
    void check_property_values();

    auto generate_quard_model_matrix()  -> glm::mat4;
    auto generate_circle_model_matrix() -> glm::mat4;

  protected:
    ShapeType _shape_type   = ShapeType::Unknow;
    Layout*   _layout       = nullptr;
    uint32_t  _x            = 0;
    uint32_t  _y            = 0;
    glm::vec3 _color;
    // INFO: default depth value is .1f, is to convience set background depth is 0.f
    float     _depth          = .1f;
    float     _rotation_angle = 0.f;

    std::vector<uint32_t> _property_values;

    //
    // features
    //
    bool _clickable = false;
  };

}}
