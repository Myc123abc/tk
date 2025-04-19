#include "tk/ui/ui.hpp"
#include "tk/ErrorHandling.hpp"

// HACK: use for background
#include "tk/tk.hpp"

#include <algorithm>
#include <memory>

namespace tk { namespace ui {

static graphics_engine::GraphicsEngine*       _engine;
static std::vector<std::unique_ptr<Layout>>   _layouts;
static std::vector<std::unique_ptr<UIWidget>> _widgets;

// HACK: tmp way
Layout* background_layout;
Button* background_picuture; // it not a button, just good way
void init(graphics_engine::GraphicsEngine* engine)
{
  _engine = engine;
  // default use onedark background
  background_layout = create_layout();
  uint32_t w, h;
  tk::get_main_window()->get_framebuffer_size(w, h);
  background_picuture = create_button(w, h, Color::OneDark);
  put(background_layout, tk::get_main_window(), 0, 0);
  put(background_picuture, background_layout, 0, 0);
  background_picuture->set_depth(0.f);
}

auto create_layout() -> Layout*
{
  return _layouts.emplace_back(std::make_unique<Layout>()).get();
}

auto create_button(uint32_t width, uint32_t height, Color color) -> Button*
{
  return dynamic_cast<Button*>(_widgets.emplace_back(std::make_unique<Button>(width, height, color)).get());
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
  _engine->render_begin();

  uint32_t width, height;
  tk::get_main_window()->get_framebuffer_size(width, height);
  background_picuture->set_width_height(width, height);

  for (auto const& layout : _layouts)
  {
    for (auto widget : layout->widgets)
    {
      glm::mat4 model;

      switch (widget->type)
      {
      case ShapeType::Unknow:
        throw_if(true, "unknow shape type of ui widget");
        break;

      case ShapeType::Quard:
        auto button = dynamic_cast<Button*>(widget);
        model = button->make_model_matrix();
        break;
      }

      // model = glm::ortho(-1.f, 1.f, 1.f, -1.f, -1.f, 100.f) * model;
      // model = glm::perspective(45.f, (float)width / height, 10000.f, 0.1f) * model;
      _engine->render_shape(widget->type, widget->get_color(), model, widget->get_depth());
    }
  }

  _engine->render_end();
}

// void UI::init(tk_context& ctx)
// {
//   _ctx = &ctx;
//   _painter.create_canvas("background")
//           .put("background", ctx.window, 0, 0)
//           .use_canvas("background");
//
//   uint32_t width, height;
//   ctx.window.get_framebuffer_size(width, height);
//   _background_id = _painter.draw_quard(0, 0, width, height, Color::OneDark);
// }
//
// void UI::destroy()
// {
// }
//
// void UI::render()
// {
//   _ctx->engine.draw();
// }
//
// void GraphicsEngine::painter_to_draw()
// {
//   uint32_t width, height;
//   _window->get_framebuffer_size(width, height);
//
//   // draw background
//   _painter.use_canvas("background");
//   _background_id = _painter.draw_quard(0, 0, width, height, Color::OneDark);
//   // draw shapes
//   _painter.use_canvas("shapes");
//   _painter.draw_quard(0, 0, 250, 250, Color::Red);
//   _painter.draw_quard(250, 0, 250, 250, Color::Green);
//   _painter.draw_quard(0, 250, 250, 250, Color::Blue);
//   _painter.draw_quard(250, 250, 250, 250, Color::Yellow);
//   _painter.generate_shape_matrix_info_of_all_canvases();
// }

// resize swapchain
  // uint32_t width, height;
  // _window->get_framebuffer_size(width, height);
  // _painter.use_canvas("background")
  //         .redraw_quard(_background_id, 0, 0, width, height, Color::OneDark);
  // _painter.generate_shape_matrix_info_of_all_canvases();
} }
