#include "tk/ui/widgets/UIWidget.hpp"
#include "tk/type.hpp"
#include "tk/ErrorHandling.hpp"

#include <SDL3/SDL_mouse.h>

using namespace tk; 

namespace tk { namespace  ui {

void UIWidget::check_property_values()
{
  switch (_shape_type) 
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

auto UIWidget::generate_model_matrix() -> glm::mat4
{
  switch (_shape_type)
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

  assert(_layout && width >= 0 && height >= 0);
  uint32_t window_width, window_height;
  _layout->get_window()->get_framebuffer_size(window_width, window_height);
  auto scale_x = (float)width / window_width;
  auto scale_y = (float)height / window_height;
  auto translate_x = (_layout->get_x() + (float)width  / 2 + _x) / ((float)window_width  / 2) - 1.f;
  auto translate_y = (_layout->get_y() + (float)height / 2 + _y) / ((float)window_height / 2) - 1.f;
  auto translate = glm::translate(glm::mat4(1.f), glm::vec3(translate_x, translate_y, 0.f));
  auto scale     = glm::scale(glm::mat4(1.f), glm::vec3(scale_x, scale_y, 1.f));
  auto rotate    = glm::rotate(glm::mat4(1.f), _rotation_angle, glm::vec3(0.f, 0.f, 1.f));
  return translate * scale *rotate;
}

auto UIWidget::generate_circle_model_matrix() -> glm::mat4
{
  auto diameter  = _property_values[0] * 2;

  assert(_layout && diameter > 0);
  uint32_t window_width, window_height;
  _layout->get_window()->get_framebuffer_size(window_width, window_height);
  auto scale_x = (float)diameter / window_width;
  auto scale_y = (float)diameter / window_height;
  auto translate_x = (_layout->get_x() + (float)diameter / 2 + _x) / ((float)window_width  / 2) - 1.f;
  auto translate_y = (_layout->get_y() + (float)diameter / 2 + _y) / ((float)window_height / 2) - 1.f;
  auto model = glm::mat4(1.f);
  model = glm::translate(model, glm::vec3(translate_x, translate_y, 0.f));
  model = glm::scale(model, glm::vec3(scale_x, scale_y, 1.f));
  return model;
}

}}
