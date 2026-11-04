#include "test_window_1.hpp"
#include "test_window_2.hpp"
#include "font_select_window.hpp"

void TestWindow1::draw() noexcept
{
  // Draw fps on title bar
  ui::draw_on_title_bar([&](float2 extent)
  {
    auto constexpr font_size = 12.f;
    ui::text(fps_text, { 8, (extent.y - font_size) / 2 }, font_size, 0x000000ff);
  });

  if (ui::get_key(ui::Key::F11).is_down())
  {
    if (ui::is_fullscreen_window())
      ui::restore_fullscreen_window();
    else
      ui::fullscreen_window();
  }
  auto extent = ui::window_drawable_extent();
  auto pos = float2{ extent.x / 2 - 50, extent.y - 50 };
  auto size = ui::window_drawable_extent();
  // ui::triangle({ size.x / 2, size.y * .1f }, size * .9f, { size.y * .1f, size.y * .9 }, 0x00ff0044, 10);

  if (ui::button("btn2", 50, 50, 100, 100, 0xffffffff, 0x00ff00ff))
    test_window_2.display_switch();
  ui::add_move_invalid_area({}, { 150, 150 });

  // silder
  // static float outline_width{};
  // silder("silder_test", 0, 90, 100, 10, 0, 0.5, outline_width);
  // ui::text(std::format("outline width : {}", outline_width), { 0, 90 }, 24, 0x00ff00ff);

  auto cfg = ui::TextConfig{};
  // cfg.outer_color = 0xff0000ff;
  // cfg.outline_width = outline_width;
  cfg.direction = ui::TextDirection::vertical;
  cfg.pos_as_baseline = true;
  auto res = ui::text("AigW.,()马钰 日本語のテスト。", 32, cfg);
  auto p1 = float2{ 30 + res.ascender, 0 };
  auto p2 = float2{ 30 + res.ascender, res.extent.y };

  ui::transform_beg(ui::Transform{}.rotate((p1 + p2) / 2, -30));
  ui::text("AigW.,()马钰 日本語のテスト。", { 30, 0 }, 32, 0x000000ff, cfg);
  ui::rectangle({ 30, 0 }, { 30 + res.extent.x, res.extent.y }, 0x00ff00ff, 1);
  ui::line({ 30 + res.ascender, 0 }, { 30 + res.ascender, res.extent.y }, 0xff0000ff);
  ui::transform_end();

  ui::check_box("check box", { 50, 150 }, 20, 0x0000ffff, font_select_window.is_shown());
}
