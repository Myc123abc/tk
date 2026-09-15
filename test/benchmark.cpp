#include "tk/tk.hpp"

using namespace tk;

auto main() -> int
{
  tk::init();

  auto wndCfg = ui::WindowConfig{};
  wndCfg.display_title_bar             = true;
  wndCfg.display_window_shadow         = true;
  wndCfg.display_wireframe_only_active = true;
  wndCfg.wireframe_color               = 0x0000ffff;

  auto is_closed = false;
  while (!is_closed)
  {
    ui::begin("Benchmark", 0, 0, 200, 200, &is_closed, wndCfg);

    ui::rectangle({}, ui::window_drawable_extent(), 0xffffffff);

    ui::end();

    tk::update();
  }

  tk::destroy();
}
