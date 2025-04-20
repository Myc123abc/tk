#include "tk/ui/UIWidget.hpp"
#include "tk/type.hpp"
#include "tk/ErrorHandling.hpp"

#include <SDL3/SDL_mouse.h>

#include <algorithm>
#include "tk/log.hpp"

using namespace tk; 

namespace tk { namespace  ui {

////////////////////////////////////////////////////////////////////////////////
//                              UIWidget
////////////////////////////////////////////////////////////////////////////////

auto UIWidget::set_type(ShapeType type)             -> UIWidget&
{
  _type = type;
  return *this;
}

auto UIWidget::set_layout(Layout* layout)           -> UIWidget&
{
  _layout = layout;
  return *this;
}

auto UIWidget::set_position(uint32_t x, uint32_t y) -> UIWidget&
{
  _x = x;
  _y = y;
  return *this;
}

auto UIWidget::set_color(Color color)               -> UIWidget&
{
  _color = color;
  return *this;
}

auto UIWidget::set_depth(float depth)               -> UIWidget&
{
  assert(depth >= 0.f && depth <= 1.f);
  _depth = depth;
  return *this;
}

void UIWidget::check_property_values()
{
  switch (_type) 
  {
  case ShapeType::Unknow:
    throw_if(true, "input unknow shape type");

  case ShapeType::Quard:
    throw_if(_property_values.size() != 2, "quard shape type only need 2 values of width and height");
    break;

  case ShapeType::Circle:
    throw_if(_property_values.size() != 1, "circle shape type only need 1 values of radius");
    break;
  } 
}

auto UIWidget::set_shape_properties(std::initializer_list<uint32_t> values) -> UIWidget&
{
  _property_values = values;
  check_property_values();
  return *this;
}

auto UIWidget::generate_model_matrix() -> glm::mat4
{
  switch (_type)
  {
  case ShapeType::Unknow:
    throw_if(true, "unknow shape type to generate model matrix");

  case ShapeType::Quard:
    return generate_quard_model_matrix();

  case ShapeType::Circle:
    return generate_circle_model_matrix();
  };
}

auto UIWidget::generate_quard_model_matrix() -> glm::mat4
{
  auto width  = _property_values[0];
  auto height = _property_values[1];

  assert(_layout && width > 0 && height > 0);
  uint32_t window_width, window_height;
  _layout->window->get_framebuffer_size(window_width, window_height);
  auto scale_x = (float)width / window_width;
  auto scale_y = (float)height / window_height;
  auto translate_x = (_layout->x + (float)width  / 2 + _x) / ((float)window_width  / 2) - 1.f;
  auto translate_y = (_layout->y + (float)height / 2 + _y) / ((float)window_height / 2) - 1.f;
  auto model = glm::mat4(1.f);
  model = glm::translate(model, glm::vec3(translate_x, translate_y, 0.f));
  model = glm::scale(model, glm::vec3(scale_x, scale_y, 1.f));
  return model;
}

auto UIWidget::generate_circle_model_matrix() -> glm::mat4
{
  auto diameter  = _property_values[0] * 2;

  assert(_layout && diameter > 0);
  uint32_t window_width, window_height;
  _layout->window->get_framebuffer_size(window_width, window_height);
  auto scale_x = (float)diameter / window_width;
  auto scale_y = (float)diameter / window_height;
  auto translate_x = (_layout->x + (float)diameter / 2 + _x) / ((float)window_width  / 2) - 1.f;
  auto translate_y = (_layout->y + (float)diameter / 2 + _y) / ((float)window_height / 2) - 1.f;
  auto model = glm::mat4(1.f);
  model = glm::translate(model, glm::vec3(translate_x, translate_y, 0.f));
  model = glm::scale(model, glm::vec3(scale_x, scale_y, 1.f));
  return model;
}

bool UIWidget::is_mouse_over()
{
  if (_layout == nullptr)
    return false;

  switch (_type)
  {
  case ShapeType::Unknow:
    throw_if(true, "unknow type of mouse over dectection");

  case ShapeType::Quard:
    return mouse_over_quard();

  case ShapeType::Circle:
    return mouse_over_circle();
  }
}

bool UIWidget::mouse_over_quard()
{ 
  float x, y;
  SDL_GetMouseState(&x, &y);

  auto width  = _property_values[0];
  auto height = _property_values[1];

  if (x > _x + _layout->x && x < _x + _layout->x + width &&
      y > _y + _layout->y && y < _y + _layout->y + height)
    return true;
  return false;
}

bool UIWidget::mouse_over_circle()
{
  float x, y;
  SDL_GetMouseState(&x, &y);

  float radius   = _property_values[0];
  float center_x = _layout->x + _x + radius;
  float center_y = _layout->y + _y + radius;

  float subtracted_x = x - center_x;
  float subtracted_y = y - center_y;

  auto d = std::sqrt(subtracted_x * subtracted_x + subtracted_y * subtracted_y);
  
  auto fmt = std::format(
      "\nradius:   {}\n"
      "center:   {},{}\n"
      "distance: {}\n"
      "sub:      {},{}\n"
      "mouse:    {},{}\n"
      , radius, center_x, center_y, d, subtracted_x, subtracted_y, x, y
      );
  log::info(fmt);

  if (d < radius)
    return true;
  return false;
}

////////////////////////////////////////////////////////////////////////////////
//                               Button
////////////////////////////////////////////////////////////////////////////////

}}
