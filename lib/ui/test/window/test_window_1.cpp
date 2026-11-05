#include "test_window_1.hpp"
#include "test_window_2.hpp"
#include "font_select_window.hpp"

void TestWindow1::draw() noexcept
{
  // draw fps on title bar
  ui::draw_on_title_bar([&](float2 extent)
  {
    auto constexpr font_size = 12.f;
    ui::text(fps_text, { 8, (extent.y - font_size) / 2 }, font_size, 0x000000ff);
  });

  // start positiont for drawing
  auto pen     = float2{ 5, 5 };
  auto padding = float2{ 5, 5 };

  // fullscreen test feature
  pen.y += ui::text("Press F11 to switch between fullscreen window and normal window.", pen, 24, 0x000000ff).extent.y;
  if (ui::get_key(ui::Key::F11).is_down())
  {
    if (ui::is_fullscreen_window())
      ui::restore_fullscreen_window();
    else
      ui::fullscreen_window();
  }
  pen.y += padding.y;

  auto check_box_cfg = ui::CheckBoxConfig
  {
    .color             = 0x2563ebff,
    .hover_color       = 0x3b82f6ff,
    .hover_inner_color = 0x3b82f6ff,
    .click_color       = 0x1d4ed8ff,
    .click_inner_color = 0x1d4ed8ff,
    .text_size         = 16,
    .text_color        = 0x000000ff,
  };

  // display test window 2
  pen.y += ui::check_box("check box - test window 2", "Display test window 2", pen, test_window_2.is_shown(), check_box_cfg).y;
  pen.y += padding.y;

  // display font select window
  pen.y += ui::check_box("check box - font select window", "Display font select window", pen, font_select_window.is_shown(), check_box_cfg).y;
  pen.y += padding.y;
}
