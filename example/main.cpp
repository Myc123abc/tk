//
// test tk library
//

#include "PlayBackButton.hpp"
#include "tk/tk.hpp"
#include "tk/log.hpp"

#include <chrono>
#include <thread>

auto playback_pos0 = glm::vec2(5, 5);
auto playback_pos1 = playback_pos0 + glm::vec2(12.5 * 1.414, 12.5);
auto playback_pos2 = playback_pos0 + glm::vec2(0, 25);
auto playback_btn  = PlayBackButton("playback_btn", playback_pos0, playback_pos1, playback_pos2, 0xffffffff, 1);

void render();

int main()
{
  // init main window and engine
  tk_init("tk", 200, 200);

  while (true)
  {
    tk_poll_events();
    auto res = tk_event_process();
    if (res == type::window::closed)
      break;
    else if (res == type::window::suspended)
      continue;

    render();
    tk_render();
  }

  tk_destroy();
}

void render()
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
    playback_btn.render();
    static bool click = {};
    if (playback_btn.button())
    {
      //log::info("click");
      click = !click;
    }
    if (click)
    {
      ui::circle({50, 50}, 10, 0x00ff00ff);
    }
    
    // playback progress
    auto playback_progree_pos = playback_pos1 + glm::vec2(5, 0);
    ui::rectangle(playback_progree_pos, playback_progree_pos + glm::vec2{ 100, 3 }, 0x808080FF );
    ui::rectangle(playback_progree_pos, playback_progree_pos + glm::vec2{ progress, 3 }, 0x0000FFFF );

    ui::end();
  }
}