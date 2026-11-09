#include "font_select_window.hpp"
#include "../font.hpp"

void FontSelectWindow::draw() noexcept
{
  auto const selector_extent = font_selector({}, 24, { 16, 16 });

  static auto selected_family = std::string{};
  static auto selected_style  = ui::FontStyle{};
  if (font_selector.is_selected())
  {
    auto const& selected_font = font_selector.selected_font();
    selected_family = selected_font.family;
    selected_style  = selected_font.style;
    info("selected font : {}", selected_family);
  }

  static auto text_limit = 320.f;
  auto const test_pos = float2{ selector_extent.x + 32, 76 };
  ui::text(std::format("limit_len = {:.0f}", text_limit), { test_pos.x, 16 }, 18, 0x000000ff);
  ui::slider("text_limit_slider", test_pos.x, 44, 400, 16, 5, 800, text_limit, ui::SliderConfig
  {
    .bar_color            = 0xd0d0d0ff,
    .bar_hover_color      = 0xc0c0c0ff,
    .progress_color       = 0x2196f3ff,
    .progress_hover_color = 0x1976d2ff,
    .rounding             = true,
    .draw_cfg             = ui::SliderConfig::CircleKnob(6, 16, 0x1976d2ff, 0x0d47a1ff),
  });

  auto text_cfg = ui::TextConfig
  {
    .family    = selected_family,
    .style     = selected_style,
    .limit_len = text_limit,
  };
  auto const text_ext = ui::text(
    "Unicode line breaking should keep English words together. "
    "中文标点不应该出现在错误的行首或行尾。"
    "日本語の句読点も正しい位置で改行する必要があります。",
    test_pos,
    24,
    0x000000ff,
    text_cfg).extent;
  ui::rectangle(test_pos, test_pos + text_ext, 0xff0000ff, 1);

  if (ui::get_key(ui::Key::F11).is_down())
    ui::is_fullscreen_window() ? ui::restore_fullscreen_window() : ui::fullscreen_window();
}
