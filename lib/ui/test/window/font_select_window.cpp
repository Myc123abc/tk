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
    // .direction = ui::TextDirection::vertical,
    .limit_len = text_limit,
  };
  auto const text_ext = ui::text(
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
    test_pos,
    24,
    0x000000ff,
    text_cfg).extent;
  ui::rectangle(test_pos, test_pos + text_ext, 0xff0000ff, 1);

  if (ui::get_key(ui::Key::F11).is_down())
    ui::is_fullscreen_window() ? ui::restore_fullscreen_window() : ui::fullscreen_window();
}
