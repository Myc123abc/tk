#include "tk/ui/UIWidget.hpp"

namespace tk { namespace  ui {

////////////////////////////////////////////////////////////////////////////////
//                              UIWidget
////////////////////////////////////////////////////////////////////////////////

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


////////////////////////////////////////////////////////////////////////////////
//                               Button
////////////////////////////////////////////////////////////////////////////////

bool Button::is_clicked() { return false; }

auto Button::make_model_matrix() -> glm::mat4
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

void Button::set_width_height(uint32_t width, uint32_t height) 
{
  _width = width;
  _height = height;
}

}}
