#include "tk/UI/UI.hpp"
#include "tk/Log.hpp"

namespace tk { namespace ui {

using namespace graphics_engine;

void UI::init(Window const& window)
{
  _engine.init(window);
  _painter.create_canvas("background")
          .put("background", window, 0, 0)
          .use_canvas("background");

  uint32_t width, height;
  window.get_framebuffer_size(width, height);
  _background_id = _painter.draw_quard(0, 0, width, height, Color::OneDark);

  _destructor.push([&]
  {
    _engine.destroy();
  });
}

void UI::destroy()
{
  _destructor.clear(); 
}

void UI::render()
{
  _engine.draw();  
}

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
}}
