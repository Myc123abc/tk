#include "Painter.hpp"
#include "ErrorHandling.hpp"
#include "Window.hpp"

#include <glm/gtc/matrix_transform.hpp>

#include <cassert>
#include <ranges>

namespace tk { namespace graphics_engine {

auto Painter::create_canvas(std::string_view name) -> Painter&
{
  throw_if(_canvases.contains(name.data()), "canvas {} is existed", name);
  _canvases.emplace(name, Canvas());
  return *this;
}

auto Painter::use_canvas(std::string_view name) -> Painter&
{
  throw_if(!_canvases.contains(name.data()), "canvas {} is not exist", name);
  _canvas = &_canvases[name.data()];
  return *this;
}

auto Painter::put(std::string_view canvas, class Window const& window, uint32_t x, uint32_t y) -> Painter&
{
  throw_if(!_canvases.contains(canvas.data()), "canvas {} is not exist", canvas);
  _canvases[canvas.data()].window = &window;
  _canvases[canvas.data()].x      = x;
  _canvases[canvas.data()].y      = y;
  return *this;
}

auto Painter::draw_quard(std::string_view name, uint32_t x, uint32_t y, uint32_t width, uint32_t height, Color color) -> Painter&
{
  assert(_canvas);

  auto it = std::ranges::find_if(_canvas->shape_infos, [name](auto const& info)
  {
    return info->name == name;
  });
  if (it != _canvas->shape_infos.end())
  {
      auto quard = dynamic_cast<QuardInfo*>(it->get());
      quard->name   = name;
      quard->x      = x;
      quard->y      = y;
      quard->color  = color;
      quard->width  = width;
      quard->height = height;
      return *this;
  }

  auto quard    = std::make_unique<QuardInfo>();
  quard->name   = name;
  quard->type   = ShapeType::Quard;
  quard->x      = x;
  quard->y      = y;
  quard->color  = color;
  quard->width  = width;
  quard->height = height;
  _canvas->shape_infos.emplace_back(std::move(quard));
  return *this;
}

auto Painter::generate_shape_matrix_info_of_all_canvases() -> Painter&
{
  for (auto const& [name, canvas] : _canvases)
  {
    auto shape_matrixs = std::vector<ShapeMatrixInfo>();

    for (auto const& info : canvas.shape_infos)
    {
      switch (info->type) 
      {
      case ShapeType::Quard:
        shape_matrixs.emplace_back(info->type, info->color, get_quard_matrix(dynamic_cast<QuardInfo const&>(*info), *canvas.window, canvas.x, canvas.y));
        break;
      }
    }
      _canvas_shape_matrix_infos[name] = std::move(shape_matrixs);
  }

  return *this;
}

auto Painter::get_quard_matrix(QuardInfo const& info, Window const& window, uint32_t x, uint32_t y) -> glm::mat4
{
  uint32_t window_width, window_height;
  window.get_framebuffer_size(window_width, window_height);
  auto scale_x = (float)info.width / window_width;
  auto scale_y = (float)info.height / window_height;
  auto translate_x = (info.x + (float)info.width / 2 + x) / ((float)window_width / 2) - 1.f;
  auto translate_y = (info.y + (float)info.height / 2 + y) / ((float)window_height / 2) - 1.f;
  auto model = glm::mat4(1.f);
  model = glm::translate(model, glm::vec3(translate_x, translate_y, 0.f));
  model = glm::scale(model, glm::vec3(scale_x, scale_y, 1.f));
  return model;
};

}}
