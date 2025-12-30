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
      auto cfg = ui::WindowConfig{};
      cfg.display_title_bar = true;
      ui::begin("wnd1", 50, 50, 200, 200, &wnd1_is_closed, cfg);
      ui::rectangle({}, { 200, 200 }, 0xffffffff);
      if (ui::is_click_on({}, { 100, 100 }))
        info("click");
      ui::rectangle({}, { 100, 100 }, 0x00ff00ff);
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
