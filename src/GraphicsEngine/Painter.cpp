#include "Painter.hpp"
#include "ErrorHandling.hpp"
#include "Window.hpp"

#include <glm/gtc/matrix_transform.hpp>

#include <cassert>

namespace tk { namespace graphics_engine {

auto Painter::create_canvas(std::string_view name, uint32_t width, uint32_t height) -> Painter&
{
  throw_if(_canvases.contains(name.data()), "canvas {} is existed", name);
  _canvases[name.data()] = { width, height };
  return *this;
}

auto Painter::use_canvas(std::string_view name) -> Painter&
{
  throw_if(!_canvases.contains(name.data()), "canvas {} is not exist", name);
  _canvas = &_canvases[name.data()];
  return *this;
}

auto Painter::draw_quard(uint32_t x, uint32_t y, uint32_t width, uint32_t height, Color color) -> Painter&
{
  assert(_canvas);
  QuardInfo quard;
  quard.type   = ShapeType::Quard;
  quard.x      = x;
  quard.y      = y;
  quard.color  = color;
  quard.width  = width;
  quard.height = height;
  _canvas->shape_infos.emplace_back(std::make_unique<QuardInfo>(quard));
  return *this;
}

//
// vertices buffer and indices buffer only use storage data of every type shapes
// different shapes use matrix to draw themselve
//
auto Painter::present(std::string_view canvas_name, Window const& window, uint32_t x, uint32_t y) -> Painter&
{
  throw_if(!_canvases.contains(canvas_name.data()), "canvas {} is not exist", canvas_name);
  auto& canvas = _canvases[canvas_name.data()];

  auto shape_matrixs = std::vector<ShapeMatrixInfo>();

  for (auto const& info : canvas.shape_infos)
  {
    switch (info->type) 
    {
    case ShapeType::Quard:
      // FIX: same shape type different color
      //      position and color should seperate
      _shape_meshs.try_emplace(info->type, create_quard(info->color));
      shape_matrixs.emplace_back(info->type, get_quard_matrix(*_canvas, dynamic_cast<QuardInfo const&>(*info), window, x, y));
      break;
    }
  }

  _canvas_shape_matrix_infos[canvas_name.data()] = shape_matrixs;

  return *this;
}

auto Painter::get_quard_matrix(Canvas const& canvas, QuardInfo const& info, Window const& window, uint32_t x, uint32_t y) -> glm::mat4
{
  auto scale_x = ((float)info.width / canvas.width) * ((float)canvas.width / window.width());
  auto scale_y = ((float)info.height / canvas.height) * ((float)canvas.height / window.height());
  auto translate_x = (info.x + x) / ((float)window.width() / 2) - 1.f;
  auto translate_y = (info.y + y) / ((float)window.height() / 2) - 1.f;
  auto model = glm::mat4(1.f);
  model = glm::translate(model, glm::vec3(translate_x, translate_y, 0.f));
  model = glm::scale(model, glm::vec3(scale_x, scale_y, 1.f));
  return model;
};

}}
