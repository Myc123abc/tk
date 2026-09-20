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

  // get layout
  auto layout = get_text_layout({ "1", "2.4", "3.1", "4", "5" }, "", {}, TextDirection::horizontal);

  // set property
  layout.inner_color = 0xffffffff;
  layout.scale = 1.f;
  layout.pos = { 50, 50 };

  // render normal
  g_ui_ctx.render_text_layout(layout);

  // render center aligment
  layout.pos.y += layout.max_height;
  layout.padding.x = 32;
  g_ui_ctx.render_text_layout(layout.center_aligment());

  // draw auxiliary lines
  auto x = layout.pos.x;
  auto w = layout.max_width + layout.padding.x;
  for (auto const& text : layout.texts)
  {
    ui::line({ x, 0 }, { x, 1000 }, 0x00ff00ff);
    ui::line({ x + w, 0 }, { x + w, 1000 }, 0x00ff00ff);
    x += w;
  }

  ui::end();
}

}