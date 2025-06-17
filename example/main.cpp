//
// test tk library
//

#include "PlayBackButton.hpp"
#include "tk/tk.hpp"
#include "tk/log.hpp"

#include <chrono>

auto playback_pos0 = glm::vec2(5, 5);
auto playback_pos1 = playback_pos0 + glm::vec2(12.5 * 1.414, 12.5);
auto playback_pos2 = playback_pos0 + glm::vec2(0, 25);
auto playback_btn  = PlayBackButton("playback_btn", playback_pos0, playback_pos1, playback_pos2, 0xffffffff, 1);
bool click = {};

auto event_process() -> type::window;
void render();

int main()
{
  try
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

      if (event_process() == type::window::closed)
        break;
      render();

      tk_render();
    }

    tk_destroy();
  }
  catch (std::exception const& e)
  {
    log::error(e.what());
    exit(EXIT_FAILURE);
  }
}

auto event_process() -> type::window
{
  using enum type::window;
  using enum type::key;
  using enum type::key_state;

  if (tk_get_key(q) == press)
    return closed;
  if (tk_get_key(space) == press)
  {
    click = !click;
    playback_btn.click();
  }

  return running;
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
    if (playback_btn.button())
    {
      click = !click;
    }
    if (click)
    {
      ui::circle({50, 50}, 10, 0x00ff00ff);
    }

    // text
    auto min = glm::vec2(2.92002106, 45.0158272);
    auto max = glm::vec2(60.8799744, 124.329453);
    ui::rectangle(min, max, 0xffffffff);
    
    // playback progress
    auto playback_progree_pos = playback_pos1 + glm::vec2(5, 0);
    ui::rectangle(playback_progree_pos, playback_progree_pos + glm::vec2{ 100, 3 }, 0x808080FF );
    ui::rectangle(playback_progree_pos, playback_progree_pos + glm::vec2{ progress, 3 }, 0x0000FFFF );

    ui::end();
  }
}