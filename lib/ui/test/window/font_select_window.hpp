#pragma once

#include "window.hpp"
#include "common.hpp"

DefWindow(FontSelectWindow);

inline FontSelectWindow font_select_window("font select window", {}, {}, wnd_cfg);
