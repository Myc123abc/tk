module;

#include <string_view>

export module tk.ui.test.window.test_window_2;

import tk.base;
import tk.ui;
import tk.ui.test.font;
import tk.ui.test.playback_button;
import tk.ui.test.window.base;
import tk.ui.test.window.common;

using namespace tk;

export class TestWindow2 : public BaseWindow<TestWindow2>
{
public:
  using BaseWindow<TestWindow2>::BaseWindow;

  void draw() noexcept;

  void init() noexcept
  {
    playback_btn.init("playback button");
    progress_tween.init(1'000'000);
    loop_trigger.init(1'000'000, true);
    circle_lerplocator.init(250'000, ui::Tween::Mode::loop);
  }

private:
  PlaybackButton  playback_btn;
  ui::Tween       progress_tween;
  ui::LoopTrigger loop_trigger;
  ui::Tween       circle_lerplocator;

  std::string_view img1 = "assets/image/test.jpg";
  std::string_view img2 = "assets/image/test.png";
};

export inline TestWindow2 test_window_2("test window 2", float2{ 200, 200 }, float2{ 0, 1080 }, wnd_cfg);
