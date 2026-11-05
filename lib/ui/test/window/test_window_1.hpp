#pragma once

#include "window.hpp"
#include "common.hpp"
#include "../fps.hpp"

#include <format>

DefWindow(TestWindow1,
  void init() noexcept
  {
    fps.init(240);
    fps.start();
  }

  void calc_fps() noexcept
  {
    static auto acc_time = 0;
    acc_time += ui::delta_time();
    if (acc_time >= 1000'000)
    {
      fps_text = std::format("{:.0f}", fps.get());
      acc_time = 0;
    }
    fps.update();
  }

private:
  Fps         fps;
  std::string fps_text{""};
);

inline TestWindow1 test_window_1("test window 1", { 200, 200 }, std::nullopt, wnd_cfg);
