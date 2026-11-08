#include "font_select_window.hpp"
#include "../font.hpp"

void FontSelectWindow::draw() noexcept
{
  font_selector({}, 24, { 16, 16 });
  if (font_selector.is_selected())
  {
    info("selected font : {}", font_selector.selected_font().family);
  }

  if (ui::get_key(ui::Key::F11).is_down())
    ui::is_fullscreen_window() ? ui::restore_fullscreen_window() : ui::fullscreen_window();
}
