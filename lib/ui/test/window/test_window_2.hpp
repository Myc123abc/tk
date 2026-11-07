#pragma once

#include "window.hpp"
#include "common.hpp"
#include "../playback_btn.hpp"

DefWindow(TestWindow2,
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
);

inline TestWindow2 test_window_2("test window 2", float2{ 200, 200 }, float2{ 0, 1080 }, wnd_cfg);
