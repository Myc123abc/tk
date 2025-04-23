#include "tk/ui/widgets/ClickableWidget.hpp"
#include "tk/ErrorHandling.hpp"

#include <SDL3/SDL_mouse.h>

namespace tk { namespace ui {

bool ClickableWidget::is_mouse_over()
{
  if (_layout == nullptr)
    return false;

  switch (_shape_type)
  {
  case ShapeType::Unknow:
    throw_if(true, "unknow type of mouse over dectection");

  case ShapeType::Quard:
    return mouse_over_quard();

  case ShapeType::Circle:
    return mouse_over_circle();
  }
}

bool ClickableWidget::mouse_over_quard()
{ 
  float x, y;
  SDL_GetMouseState(&x, &y);

  auto width  = _property_values[0];
  auto height = _property_values[1];

  auto center         = glm::vec2(_x + _layout->get_x() + width / 2.f, _y + _layout->get_y() + height / 2.f);
  auto relative_mouse = glm::vec2(x, y) - center;

  auto rotate = glm::rotate(glm::mat4(1.f), _rotation_angle, glm::vec3(0.f, 0.f, 1.f));
  auto inverse_rotate = glm::inverse(rotate);
  auto inverse_rotate_relative_mouse = inverse_rotate * glm::vec4(relative_mouse, 0.f, 0.f);

  if (inverse_rotate_relative_mouse.x > -(float)width  / 2.f &&
      inverse_rotate_relative_mouse.x <  (float)width  / 2.f &&
      inverse_rotate_relative_mouse.y > -(float)height / 2.f &&
      inverse_rotate_relative_mouse.y <  (float)height / 2.f)
    return true;
  return false;
}

bool ClickableWidget::mouse_over_circle()
{
  float x, y;
  SDL_GetMouseState(&x, &y);

  float radius   = _property_values[0];
  float center_x = _layout->get_x() + _x + radius;
  float center_y = _layout->get_y() + _y + radius;

  float subtracted_x = x - center_x;
  float subtracted_y = y - center_y;

  auto d = std::sqrt(subtracted_x * subtracted_x + subtracted_y * subtracted_y);
  if (d < radius)
    return true;
  return false;
}

}}
