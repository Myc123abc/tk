//
// test tk library
//

#include "tk/tk.hpp"
#include "tk/ui/ui.hpp"
#include "tk/log.hpp"
#include "tk/util.hpp"

#include <chrono>

using namespace tk;

void tk_init(int argc, char** argv)
{
  init_tk_context("tk", 200, 200, nullptr);
}

void tk_iterate()
{
  {
    ui::begin("test sdf", 0, 0);

    ui::polygon({ { 50, 50 }, { 150, 50 }, { 150, 100 }, { 50, 100 } }, 0x00ff00ff);

    ui::end();
  }
}

void tk_event(SDL_Event* event)
{
}

void tk_quit()
{
}
