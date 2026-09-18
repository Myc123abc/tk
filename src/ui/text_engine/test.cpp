#include "text_layout.hpp"
#include "../ui_context.hpp"

namespace tk::ui {

void test_text_layout() noexcept
{
  static auto is_closed = false;
  auto wndCfg = ui::WindowConfig{};
  wndCfg.display_title_bar             = true;
  wndCfg.display_window_shadow         = true;
  wndCfg.display_wireframe_only_active = true;
  wndCfg.wireframe_color               = 0x0000ffff;
  if (is_closed) return;
  ui::begin("test_text_layout", 0, 0, 200, 200, &is_closed, wndCfg);

  ui::rectangle({}, ui::window_drawable_extent(), 0x000000ff);

  auto layout = get_text_layout({ "abc", "efg" }, "", {}, TextDirection::horizontal);
  layout.inner_color = 0xffffffff;
  layout.scale = 1.f;
  layout.pos = { 50, 50 };
  g_ui_ctx.render_text_layout(layout);

  ui::end();
}

}