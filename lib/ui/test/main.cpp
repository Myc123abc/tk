#include <chrono>
#include <functional>

import tk.base;
import tk.flag;
import tk.hash;
import tk.log;
import tk.rect;
import tk.string_id;
import tk.ui;
import tk.ui.test.font;
import tk.ui.test.window.font_select_window;
import tk.ui.test.window.test_window_1;
import tk.ui.test.window.test_window_2;

using namespace tk;

auto timing(std::move_only_function<void()> func) noexcept
{
  auto beg = std::chrono::steady_clock::now();
  func();
  return std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - beg).count();
}

int main()
{
  tk::init();

  font_init();
  auto dur = timing([] { ui::complete_glyphs_cache_generation(); });
  tk::info("complete glyphs cache generation : {}ms", dur);

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
