#include "tk/ui/ui.hpp"
#include "tk/ErrorHandling.hpp"
#include "internal.hpp"

// HACK: use for background
#include "tk/tk.hpp"

#include <algorithm>
#include <memory>

using namespace tk;

namespace tk { namespace ui {

// HACK: tmp way
Layout* background_layout;
Button* background_picuture; // it not a button, just good way
void init(graphics_engine::GraphicsEngine* engine)
{
  auto ctx = get_ctx();
  ctx.engine = engine;
  // default use onedark background
  background_layout = create_layout();
  uint32_t w, h;
  tk::get_main_window()->get_framebuffer_size(w, h);
  background_picuture = create_button(ShapeType::Quard, Color::OneDark, {w, h});
  put(background_layout, tk::get_main_window(), 0, 0);
  put(background_picuture, background_layout, 0, 0);
  // INFO: promise background in min depth value
  background_picuture->set_depth(0.f);
}

auto create_layout() -> Layout*
{
  auto ctx = get_ctx();
  return ctx.layouts.emplace_back(std::make_unique<Layout>()).get();
}

auto create_button(ShapeType shape, Color color, std::initializer_list<uint32_t> values) -> Button*
{
  auto btn = dynamic_cast<Button*>(get_ctx().widgets.emplace_back(std::make_unique<Button>()).get());
  btn->set_type(ShapeType::Quard);
  btn->set_shape_properties(values);
  btn->set_color(color);
  return btn;
}

void put(Layout* layout, tk::Window* window, uint32_t x, uint32_t y)
{
  layout->window = window;
  layout->x      = x;
  layout->y      = y;
}

void put(UIWidget* widget, Layout* layout, uint32_t x, uint32_t y)
{
  auto it = std::ranges::find_if(layout->widgets, [widget](auto w)
  {
    return w == widget;
  });
  if (it != layout->widgets.end())
  {
    (*it)->set_position(x, y);
  }
  else
  {
    layout->widgets.push_back(widget);
    widget->set_layout(layout);
    widget->set_position(x, y);
  }
}

void render()
{
  auto ctx = get_ctx();

  uint32_t width, height;
  tk::get_main_window()->get_framebuffer_size(width, height);
  background_picuture->set_shape_properties({width, height});

  ctx.engine->render_begin();

  for (auto const& layout : ctx.layouts)
  {
    for (auto widget : layout->widgets)
    {
      glm::mat4 model;

      switch (widget->get_type())
      {
      case ShapeType::Unknow:
        throw_if(true, "unknow shape type of ui widget");
        break;

      case ShapeType::Quard:
        auto button = dynamic_cast<Button*>(widget);
        model = button->make_model_matrix();
        break;
      }

      ctx.engine->render_shape(widget->get_type(), widget->get_color(), model, widget->get_depth());
    }
  }

  ctx.engine->render_end();
}

void remove(UIWidget* widget, Layout* layout)
{
  auto it = std::ranges::find_if(layout->widgets, [widget](auto w)
  {
    return w == widget;
  });
  if (it != layout->widgets.end())
  {
    widget->remove_from_layout();
    layout->widgets.erase(it);
  }
}

}}
