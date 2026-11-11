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

  static constexpr StringLiteral Test_Cases[]
  {
    "👩‍❤️‍👩.",
    "[👩‍❤️‍👩] {😀} (🚀)",
    "A👩‍💻B [😀] C{🚀}D",
    "終] [終] 「文字」 （日本語）",
    "❤️‍🔥{OK} [👨‍👩‍👧‍👦] 👍🏽.",
    "1️⃣[🇯🇵] 🏳️‍🌈{naïveté}.",
    "a😀b c👩‍❤️‍👩d e🚀f [g] {h}",
    "Combining marks and accented letters behave as expected: Your naïveté amuses me!"
    "Unbreakable: $(12.35) 2,1234 (12)¢ 12.54¢ (s)he file(s); breakable: ()()"
    "Unbreakable pairs: ‘$9’, ‘$[’, ‘$-’, ‘-9’, ‘/9’, ‘99’, ‘,9’, ‘9%’, ‘]%’"
    "Hyphenated words are breakable: server-side"
    "Em dashes can be broken on either side, but not in the middle: A——B"
    "NOT each space is breakable, like the last one in the following sentence (in English): He did say, “He said, ‘Good morning.’ ”"
    "Regretfully it does not work for the alternative quoting style due to the ambiguity of “’” (quote and apostrophe): one should not insert normal spaces between the last two quotation marks in ‘He said, “Good morning.”’ — Of course, non-breaking spaces are OK: ‘He said, “Good morning.” ’"
    "Still, the space rule makes my favourite ellipsis style work well, like ‘once upon a time . . .’."
    "Here's an apostrophe; and there’s a right single quotation mark."
    "Spanish inverted punctuation marks are always opening (e.g.)¡Foo bar!"
    "There should be no break after Hebrew + Hyphen like א-ת, or between solidus and Hebrew like /ת."
    "Lines like this should not have their punctuation broken!… "
    "Emojis joined by ZWJs should not be broken: 👩‍❤️‍👩."
    "Breaks can occur between Chinese characters, except before or after certain punctuation marks: 「你看過《三國演義》嗎？」他問我。",
  };

  static auto case_index = 0.f;
  static auto text_size  = 24.f;
  static auto text_limit = 320.f;
  static auto tracking   = 0.f;
  static auto vertical   = true;

  auto const controls_x = selector_extent.x + 32;
  auto controls_y = 16.f;
  auto slider_cfg = ui::SliderConfig
  {
    .bar_color            = 0xd0d0d0ff,
    .bar_hover_color      = 0xc0c0c0ff,
    .progress_color       = 0x2196f3ff,
    .progress_hover_color = 0x1976d2ff,
    .rounding             = true,
    .draw_cfg             = ui::SliderConfig::CircleKnob(6, 16, 0x1976d2ff, 0x0d47a1ff),
  };
  auto slider = [&](StringID id, std::string const& label, float min, float max, float& value)
  {
    ui::text(label, { controls_x, controls_y }, 16, 0x000000ff);
    controls_y += 24;
    ui::slider(id, controls_x, controls_y, 400, 12, min, max, value, slider_cfg);
    controls_y += 28;
  };

  slider("emoji_test_case", std::format("case = {:.0f}", case_index), 0, static_cast<float>(std::size(Test_Cases) - 1), case_index);
  slider("emoji_text_size", std::format("text size = {:.0f}", text_size), 8, 96, text_size);
  slider("emoji_text_limit", std::format("limit = {:.0f} (0 disables)", text_limit), 0, 1000, text_limit);
  slider("emoji_tracking", std::format("tracking = {:.1f}", tracking), -4, 16, tracking);

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
  controls_y += ui::check_box("emoji_vertical", "Vertical", { controls_x, controls_y }, vertical, check_box_cfg).y;

  auto const selected_case = std::min(static_cast<size_t>(case_index + .5f), std::size(Test_Cases) - 1);
  auto const test_pos = float2{ controls_x, controls_y + 16 };

  auto text_cfg = ui::TextConfig
  {
    .family    = selected_family,
    .style     = selected_style,
    .direction = vertical ? ui::TextDirection::vertical : ui::TextDirection::horizontal,
    .tracking  = tracking,
    .limit_len = text_limit,
  };
  auto const text_ext = ui::text(
    Test_Cases[selected_case],
    test_pos,
    text_size,
    0x000000ff,
    text_cfg).extent;
  ui::rectangle(test_pos, test_pos + text_ext, 0xff0000ff, 1);

  if (ui::get_key(ui::Key::F11).is_down())
    ui::is_fullscreen_window() ? ui::restore_fullscreen_window() : ui::fullscreen_window();
}
