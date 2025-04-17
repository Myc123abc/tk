#include "tk/ui.hpp"

namespace tk {

auto ui::create_layout() -> Layout
{
  return Layout();
}

auto ui::create_button(uint32_t width, uint32_t height) -> Button
{
  return Button(width, height);
}

void ui::put(Layout& layout, tk::Window& window, uint32_t x, uint32_t y)
{
  layout.window = &window;
  layout.x      = x;
  layout.y      = y;
}

void ui::put(UIWidget& widget, Layout& layout, uint32_t x, uint32_t y)
{
  layout.widget_infos.emplace_back(&widget, x, y);
}

void ui::render()
{
  _engine->draw();
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
