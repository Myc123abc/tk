//
// test tk library
//

#include "PlayBackButton.hpp"
#include "tk/tk.hpp"
#include "tk/log.hpp"

#define SDL_MAIN_USE_CALLBACKS
#include <SDL3/SDL_main.h>

#include <chrono>
#include <thread>
#include <mutex>

////////////////////////////////////////////////////////////////////////////////
//                               global variable
////////////////////////////////////////////////////////////////////////////////

struct Global
{
  glm::vec2 playback_pos0 = glm::vec2(5, 5);
  glm::vec2 playback_pos1 = playback_pos0 + glm::vec2(12.5 * 1.414, 12.5);
  glm::vec2 playback_pos2 = playback_pos0 + glm::vec2(0, 25);
  PlayBackButton playback_btn;

  Global()
    : playback_btn("playback_btn", playback_pos0, playback_pos1, playback_pos2, 0xffffffff, 1)
  {}

  bool paused = false;
};

Global* global;

SDL_AppResult SDL_AppInit(void** ptr, int argc, char** argv)
{
  try
  {
    init_tk_context("tk", 200, 200);
    global = new Global();
  }
  catch (const std::exception& e)
  {
    log::error(e.what());
    return SDL_APP_FAILURE;
  }
  *ptr = global;
  return SDL_APP_CONTINUE;
}

void tk_iterate()
{
  static auto start_time = std::chrono::high_resolution_clock::now();
  auto now = std::chrono::high_resolution_clock::now();
  auto milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(now - start_time).count();
  auto progress = milliseconds % 10000 / 100;
  static bool paused = false;

  {
    ui::begin("AudioPlayer", 0, 0);

    // background
    ui::rectangle({ 0, 0 }, tk::get_main_window_extent(), 0x282C34FF);

    // playback button
    global->playback_btn.render();
    static bool click = {};
    if (global->playback_btn.button())
    {
      //log::info("click");
      click = !click;
    }
    if (click)
    {
      ui::circle({50, 50}, 10, 0x00ff00ff);
    }
    
    // playback progress
    auto playback_progree_pos = global->playback_pos1 + glm::vec2(5, 0);
    ui::rectangle(playback_progree_pos, playback_progree_pos + glm::vec2{ 100, 3 }, 0x808080FF );
    ui::rectangle(playback_progree_pos, playback_progree_pos + glm::vec2{ progress, 3 }, 0x0000FFFF );

    ui::end();
  }
}

std::mutex g_mutex;

SDL_AppResult SDL_AppIterate(void *appstate)
{
  auto global = (Global*)appstate;
  try
  {
    std::unique_lock lock(g_mutex);

    if (global->paused)
    {
      std::this_thread::sleep_for(std::chrono::milliseconds(10));
      return SDL_APP_CONTINUE;
    }

    if (tk_resize())
      return SDL_APP_CONTINUE;

    tk_iterate();

    tk_render();
  }
  catch (const std::exception& e)
  {
    log::error(e.what());
    return SDL_APP_FAILURE;
  }
  return SDL_APP_CONTINUE;
}

/*
 * WARN: event handle is thread unsafe with iterate handle
 */
SDL_AppResult SDL_AppEvent(void *appstate, SDL_Event *event)
{
  auto global = (Global*)appstate;
  try
  {
    std::unique_lock lock(g_mutex);

    switch (event->type)
    {
    case SDL_EVENT_QUIT:
      return SDL_APP_SUCCESS;
    case SDL_EVENT_WINDOW_MINIMIZED:
      global->paused = true;
      break;
    case SDL_EVENT_WINDOW_MAXIMIZED:
    case SDL_EVENT_WINDOW_RESTORED:
      global->paused = false;
      break;
    case SDL_EVENT_WINDOW_RESIZED:
      tk_set_resize();
      break;
    case SDL_EVENT_KEY_DOWN:
      if (event->key.key == SDLK_Q)
        return SDL_APP_SUCCESS;
      if (event->key.key == SDLK_SPACE)
        global->playback_btn.click();
      break;
    }

    tk_set_event(event);
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
    delete (Global*)appstate;
    tk_quit();
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