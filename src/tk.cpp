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

////////////////////////////////////////////////////////////////////////////////
//                           tk context
////////////////////////////////////////////////////////////////////////////////

struct tk_context
{
  Window         window;
  GraphicsEngine engine;

  bool           resize    = false;

  ~tk_context()
  {
    engine.destroy();
    window.destroy();
  }
};

static tk_context* tk_ctx = {};
extern struct ui_context ui_ctx;

void tk::init_tk_context(std::string_view title, uint32_t width, uint32_t height)
{
  tk_ctx = new tk_context();

  tk_ctx->window.init(title, width, height);
  tk_ctx->engine.init(tk_ctx->window);

  // init ui context
  // TODO: currently only use main window for entire ui
  ui::get_ctx()->window_extent = { width, height };
  ui::get_ctx()->engine        = &tk_ctx->engine;
}

auto tk::get_main_window_extent() -> glm::vec2
{
  uint32_t width, height;
  tk_ctx->window.get_framebuffer_size(width, height);
  return { width, height };
}

bool tk::tk_resize()
{
  auto ui_ctx = ui::get_ctx();

  // update window size
  uint32_t width, height;
  tk_ctx->window.get_framebuffer_size(width, height);
  ui_ctx->window_extent = { width, height };

  if (tk_ctx->resize)
  {
    if (width == 0 || height == 0)
    {
      std::this_thread::sleep_for(std::chrono::milliseconds(10));
      return true;
    }
    tk_ctx->engine.resize_swapchain();
    tk_ctx->resize = false;
  }
  return false;
}

void tk::tk_render()
{
  // render ui
  tk_ctx->engine.render_begin();
  ui::render();
  tk_ctx->engine.render_end();

  ui::get_ctx()->event_type = {};
}

void tk::tk_set_resize()
{
  tk_ctx->resize = true;
}

void tk::tk_set_event(SDL_Event* event)
{
  ui::get_ctx()->event_type = event->type;
}

void tk::tk_quit()
{
  delete ui::get_ctx();
  delete tk_ctx;
}