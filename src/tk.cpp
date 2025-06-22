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

void init(std::string_view title, uint32_t width, uint32_t height)
{
  tk_ctx = new tk_context();

  tk_ctx->window.init(title, width, height);
  tk_ctx->engine.init(tk_ctx->window);
  tk_ctx->window.set_resize_swapchain_fn([&] { tk_ctx->engine.resize_swapchain(); });
  tk_ctx->window.set_get_swapchain_image_size([&] { return tk_ctx->engine.get_swapchain_image_size(); });

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

inline auto suspend() -> type::window
{
  std::this_thread::sleep_for(std::chrono::milliseconds(100));
  return type::window::suspended;
}

auto event_process() -> type::window
{
  using enum type::window;

  auto& win = tk_ctx->window;
  auto ui_ctx = ui::get_ctx();

  if (win.event_process() == closed)
    return closed;

  return win.state();
}

void render()
{
  auto& engine = tk_ctx->engine;

  if (!engine.frame_begin())
    return;

  // TODO: another vec to storage glyphs and ui call text mask render then call render
  engine.text_mask_render_begin();
  ui::text_mask_render();
  engine.render_end();

  engine.sdf_render_begin();
  ui::render();
  engine.render_end();

  engine.frame_end();
}

void destroy()
{
  delete ui::get_ctx();
  delete tk_ctx;
}

}