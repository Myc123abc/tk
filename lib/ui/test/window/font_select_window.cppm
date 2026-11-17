export module tk.ui.test.window.font_select_window;

import tk.base;
import tk.ui;
import tk.ui.test.font;
import tk.ui.test.window.base;
import tk.ui.test.window.common;

using namespace tk;

export class FontSelectWindow : public BaseWindow<FontSelectWindow>
{
public:
  using BaseWindow<FontSelectWindow>::BaseWindow;

  void draw() noexcept;
};

export inline FontSelectWindow font_select_window("font select window", {}, {}, wnd_cfg);
