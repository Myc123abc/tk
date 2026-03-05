#pragma once

#include "window.hpp"

#include <thread>
#include <latch>
#include <unordered_map>
#include <unordered_set>

namespace tk { namespace renderer {

inline auto get_cursor_pos() noexcept
{
  auto p = POINT{};
  GetCursorPos(&p);
  return glm::vec<2, int>{ p.x, p.y };
}

inline auto get_monitor(HWND handle) noexcept
{
  return MonitorFromWindow(handle, MONITOR_DEFAULTTONEAREST);
}

inline auto get_virtual_screen_rect() noexcept
{
  auto rect = RECT{};
  rect.left   = GetSystemMetrics(SM_XVIRTUALSCREEN);
  rect.top    = GetSystemMetrics(SM_YVIRTUALSCREEN);
  rect.right  = rect.left + GetSystemMetrics(SM_CXVIRTUALSCREEN);
  rect.bottom = rect.top  + GetSystemMetrics(SM_CYVIRTUALSCREEN);
  return rect;
}

inline auto get_monitor_info(HWND handle) noexcept
{
  auto monitor = MonitorFromWindow(handle, MONITOR_DEFAULTTONEAREST);
  auto info = MONITORINFO{};
  info.cbSize = sizeof(info);
  GetMonitorInfoW(monitor, &info);
  return info;
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

inline auto get_window_monitor_work_rect(HWND hwnd) noexcept -> RECT
{
  return get_monitor_info(hwnd).rcWork;
}

inline auto get_window_monitor_rect(HWND hwnd) noexcept -> RECT
{
  return get_monitor_info(hwnd).rcMonitor;
}

inline auto point_on(glm::vec<2, int> const& p, glm::vec2 const& left_top, glm::vec2 const& right_bottom) noexcept
{
  return p.x >= left_top.x && p.x <= right_bottom.x && p.y >= left_top.y && p.y <= right_bottom.y;
}

inline auto point_on(glm::vec<2, int> const& p, RECT rect) noexcept
{
  return p.x >= rect.left && p.x <= rect.right && p.y >= rect.top && p.y <= rect.bottom;
}

struct WindowSnapshot
{
  HWND     handle{};
  int      x{};
  int      y{};
  uint32_t width{};
  uint32_t height{};
  bool     moving{};
  bool     resizing{};
  bool     maximized{};
  bool     move_from_maximize{};
  float    scale{};
  
  void init(Window const& window) noexcept
  {
    handle    = window.handle();
    x         = window.x;
    y         = window.y;
    width     = window.width;
    height    = window.height;
    maximized = window.maximized;
    scale     = window.scale;
  }
};

class WindowManager
{
  friend class Window;
private:
  WindowManager()                                = default;
  ~WindowManager()                               = default;
public:
  WindowManager(WindowManager const&)            = delete;
  WindowManager(WindowManager&&)                 = delete;
  WindowManager& operator=(WindowManager const&) = delete;
  WindowManager& operator=(WindowManager&&)      = delete;

  static auto instance() noexcept -> WindowManager&
  {
    static WindowManager instance;
    return instance;
  }

  enum class Message
  {
    create_window = WM_APP,
    create_fullscreen_window,
    close_window,
    close_fullscreen_window,
    minimize_window,
    maximize_window,
    restore_window,
    signal,
  };

  void init() noexcept;
  void wait_event_process_complete() const noexcept;
  void destroy() noexcept;

  static LRESULT CALLBACK wnd_proc(HWND handle, UINT msg, WPARAM w_param, LPARAM l_param) noexcept;

  auto create_fullscreen_window() noexcept -> WindowSnapshot;
  auto create_window(int x, int y, uint32_t width, uint32_t height) noexcept -> WindowSnapshot;
  void close_window(HWND handle) const noexcept;
  void close_fullscreen_window() const noexcept;
  void minimize_window(HWND handle) const noexcept;
  void maximize_window(HWND handle) const noexcept;
  void restore_window(HWND handle) const noexcept;

  auto get_window_z_orders() const noexcept -> std::vector<HWND>;
  auto get_cursor_on_window() const noexcept -> HWND;

private:
  void message_process(HWND handle, Message msg, WPARAM w_param, LPARAM l_param) noexcept;
  void msg_create_window(WPARAM w_param) noexcept;

  void update() noexcept;
  void update_monitors() noexcept;
  static auto CALLBACK enum_display_monitors(HMONITOR monitor, HDC, LPRECT, LPARAM) -> BOOL;

private:
  static constexpr wchar_t Auxiliary_Class[] = L"vn::window::WindowManager::AuxiliaryWindow";
  static constexpr wchar_t Window_Class[]    = L"vn::window::WindowManager::Window";

private:
  std::jthread                     _thread;
  DWORD                            _thread_id{};
  std::latch                       _message_queue_create_complete{ 1 };
  Window                           _fullscreen_window;
  std::unordered_map<HWND, Window> _windows;
  HANDLE                           _signal_event{};
  std::unordered_set<HWND>         _using_mouse_pass_through_windows;
  UINT_PTR                         _timer_mouse_pass_through{};
  
  struct MonitorInfo
  {
    float scale{};
    RECT  rect{};
  };
  bool                                      _update_monitors{};
  std::unordered_map<HMONITOR, MonitorInfo> _monitor_infos{};
};

inline static auto& g_wnd_mgr{ WindowManager::instance() };

}}
