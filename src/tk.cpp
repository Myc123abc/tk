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
#include "tk/log.hpp"
#include "ui/internal.hpp"

//#define SDL_MAIN_USE_CALLBACKS
#include <SDL3/SDL.h>

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
  bool           resize    = false;

  ~tk_context()
  {
    engine.destroy();
    window.destroy();
  }
};

static tk_context* tk_ctx;
extern struct ui_context ui_ctx;

void tk::init_tk_context(std::string_view title, uint32_t width, uint32_t height, void* user_data)
{
  tk_ctx = new tk_context();

  tk_ctx->user_data = user_data;
  tk_ctx->window.init(title, width, height);
  tk_ctx->engine.init(tk_ctx->window);

  // init ui context
  // TODO: currently only use main window for entire ui
  ui::get_ctx().window_extent = { width, height };
  ui::get_ctx().engine        = &tk_ctx->engine;
}

auto tk::get_user_data() -> void*
{
  return tk_ctx->user_data;
}

auto tk::get_main_window_extent() -> glm::vec2
{
  uint32_t width, height;
  tk_ctx->window.get_framebuffer_size(width, height);
  return { width, height };
}

////////////////////////////////////////////////////////////////////////////////
//                          SDL3 Callback System
////////////////////////////////////////////////////////////////////////////////

int main()
{
    try
    {
        tk_init(0, nullptr);
        SDL_Event event;
        bool run = true;
        while (run)
        {
            while (SDL_PollEvent(&event))
            {
                tk_event(&event);

                auto& ui_ctx = ui::get_ctx();

                switch (event.type)
                {
                case SDL_EVENT_QUIT:
                    run = false;
                    break;
                case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
                    if (event.window.windowID == SDL_GetWindowID(tk_ctx->window.get()))
                        break;
                    assert(false);
                case SDL_EVENT_WINDOW_MINIMIZED:
                    tk_ctx->paused = true;
                    break;
                case SDL_EVENT_WINDOW_MAXIMIZED:
                    tk_ctx->paused = false;
                    break;
                case SDL_EVENT_WINDOW_RESIZED:
                    tk_ctx->resize = true;
                    break;
                case SDL_EVENT_KEY_DOWN:
                    if (event.key.key == SDLK_Q)
                        run = false;
                    break;
                }

                ui_ctx.event_type = event.type;
            }
            
            tk_iterate();
            if (tk_ctx->paused)
            {
                SDL_Delay(10);
                continue;
            }

            auto& ui_ctx = ui::get_ctx();

            if (tk_ctx->resize)
            {
                tk_ctx->engine.resize_swapchain();
                tk_ctx->resize = false;
            }

            // update window size
            uint32_t width, height;
            tk_ctx->window.get_framebuffer_size(width, height);
            ui_ctx.window_extent = { width, height };

            // render ui
            tk_ctx->engine.render_begin();
            ui::render();
            tk_ctx->engine.render_end();

            ui_ctx.event_type = {};
        }

        tk_quit();

        ui::destroy();

        delete tk_ctx;

        SDL_Quit();
    }
    catch (const std::exception& e)
    {
        log::error(e.what());
        return 1;
    }
}
#if 0
SDL_AppResult SDL_AppInit(void**, int argc, char** argv)
{
  try
  {
    tk_init(argc, argv);
  }
  catch (const std::exception& e)
  {
    log::error(e.what());
    return SDL_APP_FAILURE;
  }
  return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppIterate(void *appstate)
{
  try
  {
    tk_iterate();

    if (tk_ctx->paused)
    {
      SDL_Delay(10);
      return SDL_APP_CONTINUE;
    }

    auto& ui_ctx = ui::get_ctx();

    if (tk_ctx->resize)
    {
      tk_ctx->engine.resize_swapchain();
      tk_ctx->resize = false;
    }

    // update window size
    uint32_t width, height;
    tk_ctx->window.get_framebuffer_size(width, height);
    ui_ctx.window_extent = { width, height };

    // render ui
    tk_ctx->engine.render_begin();
    ui::render();
    tk_ctx->engine.render_end();

    ui_ctx.event_type = {};
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

    auto& ui_ctx = ui::get_ctx();

    switch (event->type)
    {
    case SDL_EVENT_QUIT:
      return SDL_APP_SUCCESS;
    case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
      if (event->window.windowID == SDL_GetWindowID(tk_ctx->window.get()))
        return SDL_APP_SUCCESS;
      assert(false);
    case SDL_EVENT_WINDOW_MINIMIZED:
      tk_ctx->paused = true;
      break;
    case SDL_EVENT_WINDOW_MAXIMIZED:
      tk_ctx->paused = false;
      break;
    case SDL_EVENT_WINDOW_RESIZED:
      tk_ctx->resize = true;
      break;
    case SDL_EVENT_KEY_DOWN:
      if (event->key.key == SDLK_Q)
        return SDL_APP_SUCCESS;
      break;
    }

    ui_ctx.event_type = event->type;
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
  try
  {
    tk_quit();
    
    delete tk_ctx;
    
    SDL_Quit();
  }
  catch (const std::exception& e)
  {
    log::error(e.what());
    exit(EXIT_FAILURE);
  }

  if (result == SDL_APP_SUCCESS)
    exit(EXIT_SUCCESS);
  if (result == SDL_APP_FAILURE)
    exit(EXIT_FAILURE);
}
#endif