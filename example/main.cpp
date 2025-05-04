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

void tk_iterate()
{
  static auto start_time = std::chrono::high_resolution_clock::now();
  auto now = std::chrono::high_resolution_clock::now();
  auto milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(now - start_time).count();
  auto progress = milliseconds % 10000 / 100;

  //log::info("progress {}%", progress);

  {
    ui::begin(0, 0);

    // background
    ui::rectangle({ 0, 0 }, tk::get_main_window_extent(), 0x282C34FF);

    // playback button
    auto playback_right_point = glm::vec2{ 5 + 12.5 * 1.414, 17.5 };
    ui::triangle({ 5, 5 }, playback_right_point, { 5, 30 }, 0xFFFFFFFF, 1.f);

    // playback progress
    ui::rectangle({ playback_right_point.x + 5, playback_right_point.y }, { 100, 3 }, 0x808080FF );
    ui::rectangle({ playback_right_point.x + 5, playback_right_point.y }, { progress, 3 }, 0x0000FFFF );

    ui::end();
  }
}

void tk_event(SDL_Event* event)
{
}

void tk_quit()
{
}
