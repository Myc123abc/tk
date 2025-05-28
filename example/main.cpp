//
// test tk library
//

#include "tk/tk.hpp"
#include "tk/ui/ui.hpp"
#include "tk/log.hpp"
#include "tk/util.hpp"

#include <chrono>

using namespace tk;

auto playback_pos1 = glm::vec2(5, 5);
auto playback_pos2 = playback_pos1 + glm::vec2(12.5 * 1.414, 12.5);
auto playback_pos3 = playback_pos1 + glm::vec2(0, 25);

void tk_init(int argc, char** argv)
{
  init_tk_context("tk", 200, 200, nullptr);
}

void tk_iterate()
{
  {
    ui::begin("test sdf", 0, 0);

    ui::rectangle({0, 0}, tk::get_main_window_extent(), 0x282C34FF);

    ui::triangle(playback_pos1, playback_pos2, playback_pos3, 0xFFFFFFFF);

    ui::end();
  }
}

void tk_event(SDL_Event* event)
{
}

void tk_quit()
{
}
