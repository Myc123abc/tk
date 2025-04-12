#include "Window.hpp"
#include "GraphicsEngine.hpp"
#include "Log.hpp"
#include "Shape.hpp"

#define SDL_MAIN_USE_CALLBACKS
#include <SDL3/SDL_main.h>

#include <memory>

using namespace tk;
using namespace tk::graphics_engine;

struct AppContext
{
  std::unique_ptr<Window>         window;
  std::unique_ptr<GraphicsEngine> engine;
  bool                            paused = false;
};

SDL_AppResult SDL_AppInit(void **appstate, int argc, char **argv)
{
  AppContext* ctx = nullptr;
  try
  {
    ctx = new AppContext();
    ctx->window = std::make_unique<Window>(540, 540, "Breakout");
    ctx->engine = std::make_unique<GraphicsEngine>(*ctx->window.get());
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
      ctx->engine->update();
      ctx->engine->draw();
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
      ctx->engine->resize_swapchain();
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
  delete (AppContext*)appstate;
  if (result == SDL_APP_SUCCESS)
    exit(EXIT_SUCCESS);
  if (result == SDL_APP_FAILURE)
    exit(EXIT_FAILURE);
}
