#include "font_select_window.hpp"
#include "../font.hpp"

void FontSelectWindow::draw() noexcept
{
  font_selector.render({}, 24, { 16, 16 });
}
