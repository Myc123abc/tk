#include "tk/tk.hpp"
#include "window/font_select_window.hpp"
#include "window/test_window_1.hpp"
#include "window/test_window_2.hpp"
#include "font.hpp"

using namespace tk;

int main()
{
  tk::init();

  font_init();

  test_window_1.init();
  test_window_2.init();

  test_window_1.show();

  while
  (
    test_window_1.is_shown()      ||
    test_window_2.is_shown()      ||
    font_select_window.is_shown()
  )
  {
    test_window_1.calc_fps();
    test_window_1();
    test_window_2();
    font_select_window();
    tk::update();
  }

  tk::destroy();
}
