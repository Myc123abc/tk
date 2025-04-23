#include "tk/ui/ui.hpp"
#include "internal.hpp"
#include "tk/ErrorHandling.hpp"
#include "tk/tk.hpp"

#include <memory>

using namespace tk;

namespace tk { namespace ui {

void init(graphics_engine::GraphicsEngine* engine)
{
  auto ctx = get_ctx();
  ctx.engine = engine;
} 

void init_background(glm::vec3 color)
{
  auto ctx = get_ctx();

  auto layout = create_layout();

  uint32_t w, h;
  tk::get_main_window()->get_framebuffer_size(w, h);

  ctx.background = ctx.widgets.emplace_back(std::make_unique<UIWidget>(ShapeType::Quard)).get();
  ctx.background->set_shape_properties({w, h});
  ctx.background->set_color(color);

  layout->bind(get_main_window()).set_position(0, 0);
  ctx.background->bind(layout).set_position(0, 0).set_depth(0.f);
}

auto create_layout() -> Layout*
{
  auto ctx = get_ctx();
  return ctx.layouts.emplace_back(std::make_unique<Layout>()).get();
}

auto create_line(uint32_t length, uint32_t width, float angle, glm::vec3 const& color) -> Line*
{
  auto line = dynamic_cast<Line*>(get_ctx().widgets.emplace_back(std::make_unique<Line>()).get());
  line->set_shape_properties({ length, width });
  line->set_rotation_angle(angle);
  line->set_color(color);
  return line;
}

auto create_button(ShapeType shape, glm::vec3 const& color, std::initializer_list<uint32_t> values) -> Button*
{
  auto btn = dynamic_cast<Button*>(get_ctx().widgets.emplace_back(std::make_unique<Button>(shape)).get());
  btn->set_shape_properties(values);
  btn->set_color(color);
  return btn;
}

void render()
{
  auto ctx = get_ctx();

  throw_if(ctx.background == nullptr, "not init_background");

  // refresh background
  uint32_t width, height;
  tk::get_main_window()->get_framebuffer_size(width, height);

  ctx.background->set_shape_properties({width, height});

  ctx.engine->render_begin();

  for (auto const& layout : ctx.layouts)
  {
    for (auto widget : layout->get_widgets())
    {
      ctx.engine->render_shape(widget->get_shape_type(), widget->get_color(), widget->generate_model_matrix(), widget->get_depth());
    }
  }

  ctx.engine->render_end();
}

void remove(UIWidget* widget, Layout* layout)
{
  throw_if(true, "TODO: not use ui::remove now");
  // auto it = std::ranges::find_if(layout->get_widgets(), [widget](auto w)
  // {
  //   return w == widget;
  // });
  // if (it != layout->get_widgets().end())
  // {
  //   widget->remove_from_layout();
  //   layout->get_widgets().erase(it);
  // }
}

auto to_vec3(Color color) -> glm::vec3
{
  switch (color)
  {
  case Color::Red:
    return { 1.f, 0.f, 0.f };
  case Color::Green:
    return { 0.f, 1.f, 0.f };
  case Color::Blue:
    return { 0.f, 0.f, 1.f };
  case Color::Yellow:
    return { 1.f, 1.f, 0.f };
  case Color::OneDark:
    return { (float)40/255, (float)44/255, (float)52/255,};
  case Color::Grey:
    return { (float)128/255, (float)128/255, (float)128/255,};
  case Color::Unknow:
    throw_if(true, "unknow color");
    return {};
  };
  return {};
}

}}
