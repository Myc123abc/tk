#pragma once

#include "util/rect.hpp"

#include <Windows.h>
#include <ShellScalingApi.h>

#include <assert.h>
#include <string>

namespace tk::renderer {

class Monitor
{
public:
  Monitor(HWND handle) noexcept
  {
    set_info(MonitorFromWindow(handle, MONITOR_DEFAULTTOPRIMARY));
  }

  Monitor(Rect rect) noexcept
  {
    auto rc = rect.to_RECT();
    set_info(MonitorFromRect(&rc, MONITOR_DEFAULTTOPRIMARY));
  }

  Monitor(POINT point) noexcept
  {
    set_info(MonitorFromPoint(point, MONITOR_DEFAULTTOPRIMARY));
  }

  auto name()      const noexcept { return _name;      }
  auto rect()      const noexcept { return _rect;      }
  auto work_rect() const noexcept { return _work_rect; }
  auto scale()     const noexcept { return _scale;     }

private:
  void set_info(HMONITOR monitor) noexcept
  {
    auto info = MONITORINFOEXA{ sizeof(MONITORINFOEXA) };
    GetMonitorInfoA(monitor, &info);
    _name      = info.szDevice;
    _rect      = info.rcMonitor;
    _work_rect = info.rcWork;

    uint32_t dpi_x, dpi_y;
    GetDpiForMonitor(monitor, MDT_EFFECTIVE_DPI, &dpi_x, &dpi_y);
    assert(dpi_x == dpi_y);
    _scale = dpi_x / 96.f;
  }

private:
  std::string _name;
  Rect        _rect{};
  Rect        _work_rect{}; // rect exclude taskbar
  float       _scale{};
};

inline auto get_virtual_screen_rect() noexcept
{
  auto rect = Rect{};
  rect.left   = GetSystemMetrics(SM_XVIRTUALSCREEN);
  rect.top    = GetSystemMetrics(SM_YVIRTUALSCREEN);
  rect.right  = rect.left + GetSystemMetrics(SM_CXVIRTUALSCREEN);
  rect.bottom = rect.top  + GetSystemMetrics(SM_CYVIRTUALSCREEN);
  return rect;
}

inline auto get_virtual_workarea_rect() noexcept
{
  auto rect = get_virtual_screen_rect();
  auto mi   = MONITORINFO{};
  mi.cbSize = sizeof(mi);
  if (auto mon = MonitorFromWindow(nullptr, MONITOR_DEFAULTTOPRIMARY);
      GetMonitorInfoW(mon, &mi))
  {
    auto const& m = mi.rcMonitor;
    auto const& w = mi.rcWork;
    LONG inset_left   = w.left   - m.left;   if (inset_left   < 0) inset_left   = 0;
    LONG inset_top    = w.top    - m.top;    if (inset_top    < 0) inset_top    = 0;
    LONG inset_right  = m.right  - w.right;  if (inset_right  < 0) inset_right  = 0;
    LONG inset_bottom = m.bottom - w.bottom; if (inset_bottom < 0) inset_bottom = 0;

    if (inset_left   > 0) rect.left   += inset_left;
    if (inset_top    > 0) rect.top    += inset_top;
    if (inset_right  > 0) rect.right  -= inset_right;
    if (inset_bottom > 0) rect.bottom -= inset_bottom;
  }
  return rect;
}

}
