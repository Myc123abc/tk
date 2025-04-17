#include "tk/ui.hpp"
#include "tk/ErrorHandling.hpp"
#include "tk/GraphicsEngine/MaterialLibrary.hpp"

// HACK: use for background
#include "tk/tk.hpp"
#include "tk/log.hpp"

// HACK: code hierarchy need to change

// use for MaterialLibrary
using namespace tk::graphics_engine;

#include <algorithm>

namespace tk {

// HACK: tmp way
ui::Layout* background_layout;
ui::Button background_picuture; // it not a button, just good way
void ui::init(graphics_engine::GraphicsEngine* engine)
{
  _engine = engine;
  // default use onedark background
  background_layout = create_layout();
  uint32_t w, h;
  tk::get_main_window()->get_framebuffer_size(w, h);
  background_picuture = create_button(w, h);
  put(background_layout, tk::get_main_window(), 0, 0);
  put(&background_picuture, background_layout, 0, 0);
}

// FIX: how to set draw sequence?

auto ui::create_layout() -> Layout*
{
  _layouts.emplace_back(Layout());
  return &_layouts[_layouts.size() - 1];
}

auto ui::create_button(uint32_t width, uint32_t height) -> Button
{
  return Button(width, height);
}

void ui::put(Layout* layout, tk::Window* window, uint32_t x, uint32_t y)
{
  layout->window = window;
  layout->x      = x;
  layout->y      = y;
}

void ui::put(UIWidget* widget, Layout* layout, uint32_t x, uint32_t y)
{
  auto it = std::ranges::find_if(layout->widget_infos, [&](auto const& widget_info)
  {
    return widget_info.widget == widget;
  });
  if (it != layout->widget_infos.end())
  {
    it->x = x;
    it->y = y;
  }
  else
  {
    layout->widget_infos.emplace_back(widget, x, y);
    widget->set_layout(layout);
  }
}

void ui::render()
{
  _engine->render_begin();

  for (auto const& layout : _layouts)
  {
    for (auto const& widget_info : layout.widget_infos)
    {
      ShapeType type;
      Color     color;
      glm::mat4 model;

      switch (widget_info.widget->type)
      {
      case type::unknow:
        throw_if(true, "unknow type of ui widget");
        break;

      case type::ui_button:
        type  = ShapeType::Quard;
        color = Color::Blue;
        auto button = dynamic_cast<Button*>(widget_info.widget);
  log::info("win: {}", (void*)tk::get_main_window());
  log::info("win: {}", (void*)button->_layout->window);
        model = button->make_model_matrix();
        break;
      }

      _engine->render_shape(type, color, model);
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
}
