module;

#include <format>
#include <string>

export module tk.ui.test.window.test_window_1;

import tk.base;
import tk.ui;
import tk.ui.test.fps;
import tk.ui.test.window.base;
import tk.ui.test.window.common;

using namespace tk;

export class TestWindow1 : public BaseWindow<TestWindow1>
{
public:
  using BaseWindow<TestWindow1>::BaseWindow;

  void draw() noexcept;

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
  std::string fps_text{ "" };
};

export inline TestWindow1 test_window_1("test window 1", {}, {}, wnd_cfg);
