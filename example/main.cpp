#include "tk/Log.hpp"
#include "tk/UI/UI.hpp"
#include "tk/tk.hpp"

#define SDL_MAIN_USE_CALLBACKS
#include <SDL3/SDL_main.h>

using namespace tk;
using namespace tk::ui;

struct AppContext
{
  ~AppContext()
  {
    ctx.destroy();
  }

  tk_context ctx;
  // Layout layout; 
  // Button button; 
  bool   paused = false;
};

SDL_AppResult SDL_AppInit(void **appstate, int argc, char **argv)
{
  AppContext* ctx = nullptr;
  try
  {
    ctx = new AppContext();

    ctx->ctx.init("Breakout", 1000, 1000);
    UI::init(ctx->ctx);

    // layout default create bound on main window
    // ctx->layout = UI::create_layout();
    // create button with width and height
    // ctx->button = UI::create_button(100, 100);
  }
  catch (const std::exception& e)
  {
    log::error(e.what());
    return SDL_APP_FAILURE;
  }
  *appstate = ctx;
  return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppIterate(void *appstate)
{
  auto ctx = (AppContext*)appstate;
  try
  {
    if (!ctx->paused)
    {
      // ctx->layout.put_on("main window", )
      UI::render();
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
  auto ctx = (AppContext*)appstate;
  try
  {
    switch (event->type)
    {
    case SDL_EVENT_QUIT:
      return SDL_APP_SUCCESS;
    case SDL_EVENT_WINDOW_MINIMIZED:
      ctx->paused = true;
      break;
    case SDL_EVENT_WINDOW_MAXIMIZED:
      ctx->paused = false;
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
  UI::destroy();
  delete (AppContext*)appstate;
  if (result == SDL_APP_SUCCESS)
    exit(EXIT_SUCCESS);
  if (result == SDL_APP_FAILURE)
    exit(EXIT_FAILURE);
}
