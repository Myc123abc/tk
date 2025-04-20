#include "tk/ui/ui.hpp"
#include "internal.hpp"

// HACK: use for background
#include "tk/tk.hpp"

#include <algorithm>
#include <memory>

using namespace tk;

namespace tk { namespace ui {

void init(graphics_engine::GraphicsEngine* engine)
{
  auto ctx = get_ctx();

  ctx.engine = engine;

  // 
  // create background
  //
  auto layout = create_layout();

  uint32_t w, h;
  tk::get_main_window()->get_framebuffer_size(w, h);

  ctx.background = ctx.widgets.emplace_back(std::make_unique<UIWidget>()).get();
  ctx.background->set_shape_type(ShapeType::Quard);
  ctx.background->set_shape_properties({w, h});
  ctx.background->set_color(to_vec3(Color::OneDark));

  put(layout, tk::get_main_window(), 0, 0);
  put(ctx.background, layout, 0, 0);

  // promise background in min depth value
  ctx.background->set_depth(0.f);
}

auto create_layout() -> Layout*
{
  auto ctx = get_ctx();
  return ctx.layouts.emplace_back(std::make_unique<Layout>()).get();
}

auto create_button(ShapeType shape, glm::vec3 const& color, std::initializer_list<uint32_t> values) -> Button*
{
  auto btn = dynamic_cast<Button*>(get_ctx().widgets.emplace_back(std::make_unique<Button>()).get());
  btn->set_shape_type(shape);
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

// FIX: if one widget put on multiple layout will be what?
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

  // refresh background
  uint32_t width, height;
  tk::get_main_window()->get_framebuffer_size(width, height);
  ctx.background->set_shape_properties({width, height});

  ctx.engine->render_begin();

  for (auto const& layout : ctx.layouts)
  {
    for (auto widget : layout->widgets)
    {
      ctx.engine->render_shape(widget->get_shape_type(), widget->get_color(), widget->generate_model_matrix(), widget->get_depth());
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
