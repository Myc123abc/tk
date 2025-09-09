//
// init tk context and handle events and other
// wrapper callback system of sdl3 callback system
//

#include "tk/tk.hpp"
#include "GraphicsEngine/GraphicsEngine.hpp"
#include "ui/internal.hpp"

using namespace tk;
using namespace tk::graphics_engine;

namespace tk 
{

struct tk_context
{
  Window         window;
  GraphicsEngine engine;

  void destroy()
  {
    engine.destroy();
    window.destroy();
  }
};

static tk_context* tk_ctx{};
extern struct ui_context ui_ctx;

void init(std::string_view title, uint32_t width, uint32_t height)
{
  tk_ctx = new tk_context();

  tk_ctx->window.init(title, width, height);
  tk_ctx->engine.init(tk_ctx->window);
  tk_ctx->window.set_engine(&tk_ctx->engine);

  tk_ctx->window.set_state(type::WindowState::running); // set running to avoid event process always jump the rendering
                                                        // the first rendered frame will also show the window
  
  // init ui context
  // TODO: currently only use main window for entire ui
  ui::get_ctx()->window_extent = { width, height };
  ui::get_ctx()->engine        = &tk_ctx->engine;
  ui::get_ctx()->window        = &tk_ctx->window;
}

auto get_window_size() -> glm::vec2
{
  return tk_ctx->window.get_framebuffer_size();
}

auto event_process() -> type::WindowState
{
  auto& win = tk_ctx->window;
  win.event_process();

  ui::event_process();

  return win.state();
}

auto get_key(type::Key k) -> type::KeyState
{
  return tk_ctx->window.get_key(k);
}

void render()
{
  auto& engine = tk_ctx->engine;

  if (!engine.frame_begin())
  {
    ui::clear();
    return;
  }

  engine.sdf_render_begin();
  ui::render();
  engine.render_end();

  engine.frame_end();

  static bool is_first_frame = true;
  if (is_first_frame)
  {
    tk_ctx->engine.wait_device_complete();
    tk_ctx->window.show();
    is_first_frame = false;
  }
}

void destroy()
{
  delete ui::get_ctx();
  tk_ctx->destroy();
  delete tk_ctx;
}

void load_fonts(std::vector<std::string_view> fonts)
{
  tk_ctx->engine.load_fonts(fonts);
}

}