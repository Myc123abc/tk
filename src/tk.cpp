//
// init tk context and handle events and other
// wrapper callback system of sdl3 callback system
//
// HACK:
// 1. while, use log in internal callback functions is storage...
//

#include "tk/tk.hpp"
#include "tk/Window.hpp"
#include "tk/GraphicsEngine/GraphicsEngine.hpp"
#include "tk/log.hpp"

#define SDL_MAIN_USE_CALLBACKS
#include <SDL3/SDL_main.h>

using namespace tk;
using namespace tk::graphics_engine;

////////////////////////////////////////////////////////////////////////////////
//                          tk Callback System
////////////////////////////////////////////////////////////////////////////////

extern void tk_init(int argc, char** argv);
extern void tk_iterate();
extern void tk_event(SDL_Event* event);
extern void tk_quit();

////////////////////////////////////////////////////////////////////////////////
//                           tk context
////////////////////////////////////////////////////////////////////////////////

struct tk_context
{
  void*          user_data = nullptr;

  Window         window;
  GraphicsEngine engine;

  bool           paused    = false;

  ~tk_context()
  {
    engine.destroy();
    window.destroy();
  }
};

static tk_context* tk_ctx;

void tk::init_tk_context(std::string_view title, uint32_t width, uint32_t height, void* user_data)
{
  tk_ctx = new tk_context();

  tk_ctx->user_data = user_data;
  tk_ctx->window.init(title, width, height);
  tk_ctx->engine.init(tk_ctx->window);
}

auto tk::get_user_data() -> void*
{
  return tk_ctx->user_data;
}

auto tk::get_main_window() -> Window&
{
  return tk_ctx->window;
}

////////////////////////////////////////////////////////////////////////////////
//                          SDL3 Callback System
////////////////////////////////////////////////////////////////////////////////

SDL_AppResult SDL_AppInit(void**, int argc, char** argv)
{
  tk_init(argc, argv);
  return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppIterate(void *appstate)
{
  try
  {
    tk_iterate();

    if (!tk_ctx->paused)
    {
      ui::render();
    }
  }
  catch (const std::exception& e)
  {
    log::error(e.what());
    return SDL_APP_FAILURE;
  }
  return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppEvent(void *appstate, SDL_Event *event)
{
  try
  {
    tk_event(event);

    switch (event->type)
    {
    case SDL_EVENT_QUIT:
      return SDL_APP_SUCCESS;
    case SDL_EVENT_WINDOW_MINIMIZED:
      tk_ctx->paused = true;
      break;
    case SDL_EVENT_WINDOW_MAXIMIZED:
      tk_ctx->paused = false;
      break;
    case SDL_EVENT_WINDOW_RESIZED:
      // ctx->engine.resize_swapchain();
      break;
    case SDL_EVENT_KEY_DOWN:
      if (event->key.key == SDLK_Q)
        return SDL_APP_SUCCESS;
      break;
    }
  }
  catch (const std::exception& e)
  {
    log::error(e.what());
    return SDL_APP_FAILURE;
  }
  return SDL_APP_CONTINUE;
}

void SDL_AppQuit(void *appstate, SDL_AppResult result)
{
  tk_quit();

  delete tk_ctx;

  if (result == SDL_APP_SUCCESS)
    exit(EXIT_SUCCESS);
  if (result == SDL_APP_FAILURE)
    exit(EXIT_FAILURE);
}
