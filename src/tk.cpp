//
// init tk context and handle events and other
// wrapper callback system of sdl3 callback system
//
// HACK:
// 1. while, use log in internal callback functions is strange...
//
// FIX:
// 1. should not use try catch in internal callback system!
//

#include "tk/tk.hpp"
#include "tk/GraphicsEngine/GraphicsEngine.hpp"
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

void init(std::string_view title, uint32_t width, uint32_t height, std::vector<std::string_view> const& fonts)
{
  tk_ctx = new tk_context();

  tk_ctx->window.init(title, width, height);
  tk_ctx->engine.init(tk_ctx->window);
  tk_ctx->window.set_engine(&tk_ctx->engine);
  
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

auto event_process() -> type::window
{
  auto& win = tk_ctx->window;
  win.event_process();

  ui::event_process();

  return win.state();
}

auto get_key(type::key k) -> type::key_state
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
}

void destroy()
{
  delete ui::get_ctx();
  tk_ctx->destroy();
  delete tk_ctx;
}

}