//
// test tk library
//
// tk use SDL3 callback system to organize program,
// and using internal variables to process graphics engine initialization,
// event handles and other.
//

#include "tk/tk.hpp"
#include "tk/ui/ui.hpp"
#include "tk/log.hpp"

#include <chrono>

using namespace tk;

void tk_init(int argc, char** argv)
{
  init_tk_context("tk", 200, 200, nullptr);
}

auto playback_pos1 = glm::vec2(5, 5);
auto playback_pos2 = playback_pos1 + glm::vec2(12.5 * 1.414, 12.5);
auto playback_pos3 = playback_pos1 + glm::vec2(0, 25);
auto pause_pos     = glm::vec2(playback_pos2.x, playback_pos3.y);

auto pause_button() -> bool
{
  auto width = (pause_pos.x - playback_pos1.x) / 5 * 2;
  ui::rectangle(playback_pos1, playback_pos1 + glm::vec2{ width, pause_pos.y } , 0xFFFFFFFF, 1.f);
  auto pos = glm::vec2{pause_pos.x - width, playback_pos1.y};
  ui::rectangle(pos, pos + glm::vec2{ width, pause_pos.y }, 0xFFFFFFFF, 1.f);
  return ui::click_area("pause button", playback_pos1, pause_pos);
}

void tk_iterate()
{
  static auto start_time = std::chrono::high_resolution_clock::now();
  auto now = std::chrono::high_resolution_clock::now();
  auto milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(now - start_time).count();
  auto progress = milliseconds % 10000 / 100;
  static bool paused = false;

  //log::info("progress {}%", progress);

  {
    ui::begin("AudioPlayer", 0, 0);

    // background
    ui::rectangle({ 0, 0 }, tk::get_main_window_extent(), 0x282C34FF);

    // playback button
    if (paused)
      paused = !pause_button();
    else
      paused = ui::button("playback button", type::shape::triangle, { playback_pos1, playback_pos2, playback_pos3 }, 0xFFFFFFFF, 1.f);
      
    // playback progress
    auto playback_progree_pos = playback_pos2 + glm::vec2(5, 0);
    ui::rectangle(playback_progree_pos, playback_progree_pos + glm::vec2{ 100, 3 }, 0x808080FF );
    ui::rectangle(playback_progree_pos, playback_progree_pos + glm::vec2{ progress, 3 }, 0x0000FFFF );

    ui::end();
  }
}

void tk_event(SDL_Event* event)
{
}

void tk_quit()
{
}
