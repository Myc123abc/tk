#include "tk/tk.hpp"

using namespace tk;

int main()
{
  tk::init();

  auto wnd1_is_closed = false;
  auto wnd2_is_closed = false;
  while (!wnd1_is_closed || !wnd2_is_closed)
  {
    if (!wnd1_is_closed)
    {
      ui::begin("wnd1", 50, 50, 200, 200, &wnd1_is_closed);
      ui::rectangle({}, { 200, 200 }, 0xffffffff);
      ui::end();
    }

    if (!wnd2_is_closed)
    {
      ui::begin("wnd2", 100, 100, 200, 200, &wnd2_is_closed);
      ui::rectangle({}, { 200, 200 }, 0x000000ff);
      ui::end();
    }

    ui::render();
  }

  tk::destroy();
}
