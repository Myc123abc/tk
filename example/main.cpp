//
// test tk library
//

#include "PlayBackButton.hpp"
#include "tk/tk.hpp"
#include "tk/log.hpp"

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
};

////////////////////////////////////////////////////////////////////////////////
//                               tk callback
////////////////////////////////////////////////////////////////////////////////

void tk_init(int argc, char** argv)
{
  init_tk_context("tk", 200, 200, new Global());
}

void tk_iterate()
{
  auto global = (Global*)get_user_data();

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

void tk_event(SDL_Event* event)
{
  auto global = (Global*)get_user_data();
  switch (event->type)
  {
  case SDL_EVENT_KEY_DOWN:
      if (event->key.key == SDLK_SPACE)
        global->playback_btn.click();
        break;
      break;
  }
}

void tk_quit()
{
  delete (Global*)get_user_data();;
}