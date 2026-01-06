#include "tk/tk.hpp"

using namespace tk;

int main()
{
  tk::init();

  auto wnd1_is_closed = false;
  auto wnd2_is_closed = false;
  while (!wnd1_is_closed || !wnd2_is_closed)
  {
    auto cfg = ui::WindowConfig{};
    cfg.display_title_bar = true;

    if (!wnd1_is_closed)
    {
      ui::begin("wnd1", 50, 50, 200, 200, &wnd1_is_closed, cfg);
      ui::rectangle({}, ui::window_extent(), 0xffffffff);
      if (ui::button("btn1", 0, 0, 100, 100, 0x00ff00ff, 0x0000ffff))
        info("click 11");
      if (ui::button("btn2", 50, 50, 100, 100, 0x00ff00ff, 0x0000ffff))
        info("click 22");
      ui::end();
    }

    if (!wnd2_is_closed)
    {
      ui::begin("wnd2", 100, 100, 200, 200, &wnd2_is_closed, cfg);
      ui::rectangle({}, ui::window_extent(), 0x000000ff);
      if (ui::button("btn1", 0, 0, 100, 100, 0xeeeeeeff, 0xcececeff, 0xb0b0b0ff))
        info("click 1");
      if (ui::button("btn2", 50, 50, 100, 100, 0xffffffff, 0xddddddff, 0xb0b0b0ff))
        info("click 2");
      ui::end();
    }

    ui::render();
  }

  tk::destroy();
}
