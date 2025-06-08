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
#include "tk/Window.hpp"
#include "tk/GraphicsEngine/GraphicsEngine.hpp"
#include "ui/internal.hpp"

#include <thread>
#include <chrono>

using namespace tk;
using namespace tk::graphics_engine;

namespace tk 
{

struct tk_context
{
  Window         window;
  GraphicsEngine engine;

  ~tk_context()
  {
    engine.destroy();
    window.destroy();
  }
};

static tk_context* tk_ctx = {};
extern struct ui_context ui_ctx;

void tk_init(std::string_view title, uint32_t width, uint32_t height)
{
  tk_ctx = new tk_context();

  tk_ctx->window.init(title, width, height);
  tk_ctx->engine.init(tk_ctx->window);

  // init ui context
  // TODO: currently only use main window for entire ui
  ui::get_ctx()->window_extent = { width, height };
  ui::get_ctx()->engine        = &tk_ctx->engine;
  ui::get_ctx()->window        = &tk_ctx->window;
}

void tk_poll_events()
{
  poll_events();
}

auto get_main_window_extent() -> glm::vec2
{
  return tk_ctx->window.get_framebuffer_size();
}

auto tk_event_process() -> type::window
{
  using enum type::window;

  auto& win = tk_ctx->window;
  auto ui_ctx = ui::get_ctx();

  if (win.is_closed())
    return closed;

  ui_ctx->window_extent = win.get_framebuffer_size();
  
  if (ui_ctx->window_extent.x == 0 || ui_ctx->window_extent.y == 0)
  {
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    return suspended;
  }
  
  if (win.is_resized())
  {
    if (ui_ctx->window_extent.x == 0 || ui_ctx->window_extent.y == 0)
    {
      std::this_thread::sleep_for(std::chrono::milliseconds(10));
      return suspended;
    }
    tk_ctx->engine.resize_swapchain();
  }

  if (win.is_minimized())
  {
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    return suspended;
  }

  return running;
}

void tk_render()
{
  tk_ctx->engine.render_begin();
  ui::render();
  tk_ctx->engine.render_end();
}

void tk_destroy()
{
  delete ui::get_ctx();
  delete tk_ctx;
}

}