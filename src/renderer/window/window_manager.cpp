#include "window_manager.hpp"
#include "util/error_handling.hpp"
#include "../../ui/ui_context.hpp"
#include "../renderer/renderer.hpp"
#include "monitor.hpp"
#include "compositor.hpp"

#include <shellscalingapi.h>

using namespace tk;
using namespace tk::ui;
using namespace tk::renderer;

namespace {

auto to_64_bits(uint32_t x, uint32_t y) noexcept
{
  return static_cast<uint64_t>(x) << 32 | y;
}

auto to_32_bits(uint64_t x) noexcept -> std::pair<uint32_t, uint32_t>
{
  return { static_cast<uint32_t>(x >> 32), static_cast<uint32_t>(x & 0xffffffff) };
}

void set_cursor(HWND handle, ResizeType type) noexcept
{
  using enum ResizeType;
  auto cursor = IDC_ARROW;
  switch (type)
  {
  case top:
  case bottom:
    cursor = IDC_SIZENS;
    break;
  case left:
  case right:
    cursor = IDC_SIZEWE;
    break;
  case right_top:
  case left_bottom:
    cursor = IDC_SIZENESW;
    break;
  case left_top:
  case right_bottom:
    cursor = IDC_SIZENWSE;
    break;
  case none:
    break;
  }
  SetClassLongPtrA(handle, GCLP_HCURSOR, reinterpret_cast<LONG_PTR>(LoadCursorA(nullptr, cursor)));
}

}

namespace tk::renderer {

void WindowManager::init() noexcept
{
  g_compositor.init();

  // enable DPI awareness
  SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);

  // register window class
  auto wnd_class = WNDCLASSEXW{};
  wnd_class.cbSize        = sizeof(wnd_class);
  wnd_class.hInstance     = GetModuleHandleW(nullptr);
  wnd_class.hCursor       = LoadCursorA(nullptr, IDC_ARROW);
  wnd_class.lpszClassName = Auxiliary_Class;
  wnd_class.lpfnWndProc   = DefWindowProcW;
  err_if(!RegisterClassExW(&wnd_class), "failed register class");
  wnd_class.lpszClassName = Window_Class;
  wnd_class.lpfnWndProc   = wnd_proc;
  err_if(!RegisterClassExW(&wnd_class), "failed register class");
  wnd_class.lpszClassName = Blur_Class;
  wnd_class.lpfnWndProc   = blur_wnd_proc;
  err_if(!RegisterClassExW(&wnd_class), "failed register class");
}

void WindowManager::message_process() noexcept
{
  MSG msg{};
  while (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE))
  {
    TranslateMessage(&msg);
    DispatchMessageW(&msg);
    message_process(msg);
    update();
  }
}

void WindowManager::destroy() noexcept
{
  g_compositor.destroy();
}

LRESULT CALLBACK WindowManager::blur_wnd_proc(HWND handle, UINT msg, WPARAM w_param, LPARAM l_param) noexcept
{
  static auto& wnd_mgr   = WindowManager::instance();
  static auto& wnds      = wnd_mgr._windows;
  static auto& blur_wnds = wnd_mgr._blur_windows;

  switch (msg)
  {
  case WM_WINDOWPOSCHANGED:
  {
    // avoid window hide lead other blur windows z-order changed
    if (blur_wnds.contains(handle))
      wnds[blur_wnds[handle]].keep_blur_window_behind();
    break;
  }
  }

  return DefWindowProcW(handle, msg, w_param, l_param);
}

LRESULT CALLBACK WindowManager::wnd_proc(HWND handle, UINT msg, WPARAM w_param, LPARAM l_param) noexcept
{
  static auto& wnd_mgr = WindowManager::instance();
  static auto& windows = wnd_mgr._windows;

  static auto last_cursor_pos                    = glm::vec<2, int>{};
  static auto left_button_down_window_cursor_pos = glm::vec<2, int>{};
  static auto last_resize_type                   = ResizeType{};
  static auto left_button_down_resize_type       = ResizeType{};
  static auto left_button_down                   = false;

  static auto finish_moving_or_resizing = [&](HWND handle)
  {
    ReleaseCapture();
    ClipCursor(nullptr);

    left_button_down = false;

    if (windows.contains(handle))
    {
      auto& window = windows[handle];
      if (window.is_moving())
        window.move_end();
      else if (window.is_resizing())
        window.resize_end();
    }
  };

  switch (msg)
  {
  case WM_CLOSE:
  {
    g_ui_ctx.close_window(handle);
    return 0;
  }

  case WM_DISPLAYCHANGE:
  {
    wnd_mgr._update_monitors = true;
    return 0;
  }

  case WM_SETFOCUS:
  case WM_ACTIVATE:
  case WM_SIZE:
  case WM_MOVE:
  {
    if (windows.contains(handle))
      windows[handle].keep_blur_window_behind();
    break;
  }

  case WM_SYSCOMMAND:
  {
    if (w_param == SC_RESTORE)
    {
      if (windows.contains(handle))
      {
        auto& wnd = windows[handle];
        if (wnd._blur_window)
        {
          ShowWindow(wnd._blur_window, SW_RESTORE);
          wnd.keep_blur_window_behind();
        }
      }
    }
    break;
  }

  case WM_WINDOWPOSCHANGED:
  {
    if (windows.contains(handle))
      windows[handle].keep_blur_window_behind();

    if (IsIconic(handle)) return 0;
    auto info = reinterpret_cast<WINDOWPOS*>(l_param);
    wnd_mgr._window_change_size[handle] = { info->x, info->y, info->cx + info->x, info->cy + info->y };
    return 0;
  }

  case WM_LBUTTONDOWN:
  {
    SetCapture(handle);
    auto& window = windows[handle];
    last_cursor_pos                    = get_cursor_pos();
    left_button_down_window_cursor_pos = window.cursor_pos();

    // only moving in cursor valid areas
    if (!window._move_from_maximize &&
      std::ranges::any_of(window._move_invalid_areas, [](auto rect) { return point_in_with_bounding(left_button_down_window_cursor_pos, rect); }))
      break;

    left_button_down_resize_type = window.get_resize_type(left_button_down_window_cursor_pos);
    left_button_down             = true;
    break;
  }

  case WM_MOUSEMOVE:
  {
    auto& window     = wnd_mgr._windows[handle];
    auto  cursor_pos = window.cursor_pos();
    auto const& cfg  = window._cfg;

    // update cursor
    if (auto type = window.get_resize_type(cursor_pos); !cfg.no_resize && type != last_resize_type && !window.is_fullscreen())
    {
      last_resize_type = type;
      set_cursor(handle, type);
    }

    // update window mode for mouse pass through
    if (!window.is_resizing() && !window.is_moving() &&
        !wnd_mgr._using_mouse_pass_through_windows.contains(handle) &&
        window.is_mouse_pass_through_area())
    {
      auto style = GetWindowLong(handle, GWL_EXSTYLE);
      SetWindowLongPtrA(handle, GWL_EXSTYLE, style | WS_EX_TRANSPARENT | WS_EX_LAYERED);
      wnd_mgr._using_mouse_pass_through_windows.emplace(handle); 
      if (!wnd_mgr._timer_mouse_pass_through)
        wnd_mgr._timer_mouse_pass_through = SetTimer(nullptr, 0, USER_TIMER_MINIMUM, nullptr);
    }

    auto pos = get_cursor_pos();
    if (left_button_down)
    {
      // limit cursor move area
      auto rect = get_virtual_workarea_rect();
      ClipCursor(&rect);

      // moving
      if (left_button_down_resize_type == ResizeType::none)
      {
        if (!cfg.no_move)
        {
          if (window.is_maximized())
          {
            auto pos = window.move_from_maximize();
            left_button_down_window_cursor_pos.x  = pos.x;
            left_button_down_window_cursor_pos.y += pos.y;
          }
          auto move_pos = pos - left_button_down_window_cursor_pos;
          window.move_with_pos(move_pos.x, move_pos.y);
          // resize window when moving window between different scale of monitors
          wnd_mgr.update_monitor(handle, pos, left_button_down_window_cursor_pos);
        }
      }
      // resizing
      else
      {
        if (!cfg.no_resize)
        {
          auto offset = pos - last_cursor_pos;
          window.adjust_offset(left_button_down_resize_type, pos, offset.x, offset.y);
          window.resize(left_button_down_resize_type, offset.x, offset.y);
        }
      }
    }
    last_cursor_pos = pos;
    break;
  }

  case WM_LBUTTONUP:
  {
    finish_moving_or_resizing(handle);
    break;
  }

  case WM_CANCELMODE:
  {
    if (LOWORD(w_param) == WA_INACTIVE)
    {
      finish_moving_or_resizing(handle);
      g_ui_ctx.clear_state();
    }
    break;
  }

  }

  return DefWindowProcW(handle, msg, w_param, l_param);
}

void WindowManager::update() noexcept
{
  if (_update_monitors)
  {
    _update_monitors = false;

    // update windows
    for (auto& [handle, window] : _windows)
    {
      auto rect            = RECT{};
      auto monitor         = Monitor{ handle };
      auto is_same_monitor = window.monitor() == monitor.name();

      // minimize window process
      if (IsIconic(handle))
      {
        ShowWindow(handle, SW_SHOWNOACTIVATE);
        GetWindowRect(handle, &rect);
        window.cancel_fullscreen_maximize(rect, monitor.scale());
        window.minimize();
        window.set_monitor(monitor.name());
        continue;
      }

      GetWindowRect(handle, &rect);

      // maximize window process
      if (window.is_maximized())
      {
        if (is_same_monitor)
        {
          // because some device OS get rect not really the monitor's rcWork,
          // so I need to get it manually
          window.update_by_rect(Monitor{ rect }.work_rect(), monitor.scale());
          continue;
        }
      }

      // fullscreen window process
      if (window.is_fullscreen())
      {
        if (is_same_monitor)
        {
          window.update_by_rect(Monitor{ rect }.rect(), monitor.scale());
          continue;
        }
      }
      
      // check whether the monitor of the window is removed
      if (!is_same_monitor)
      {
        if (window.is_maximized())
          window.cancel_maximize(rect, monitor.scale());
        else if (window.is_fullscreen())
          window.cancel_fullscreen(rect, monitor.scale());
        else
          window.update_by_real_rect(rect, monitor.scale());

        // update monitor
        window.set_monitor(monitor.name());
        continue;
      }

      // resize window if scale or rect changed
      if (monitor.scale() != window.scale() || window.real_rect() != rect)
        window.update_by_real_rect(rect, monitor.scale());
    }
    update_fullscreen_window();
  }

  // reset window position avoid OS move window
  if (!_window_change_size.empty())
  {
    for (auto [handle, rect] : _window_change_size)
    {
      if (!_windows.contains(handle)) continue;
      auto& window = _windows[handle];
      if (window.real_rect() != rect)
      {
        // the suck Windows operate system in some device have wrong monitor change process
        // when monitor change, the rcWork and rcMonitor is not same at first time
        // so I must recheck the right rcWork to promise my maximize window is correct
        auto mi = Monitor{ handle };
        if (window.is_maximized() && window._rect == mi.rect() && mi.rect() != mi.work_rect())
          window.update_by_rect(mi.work_rect(), mi.scale());
        else
          window.reset_pos_size();
      }
    }
    _window_change_size.clear();
  }
}

void WindowManager::resize_blur_window(HWND handle, RECT rect) noexcept
{
  if (_windows.contains(handle))
    _windows[handle].resize_blur_window(rect);
}

void WindowManager::update_monitor(HWND handle, glm::vec2 cursor_pos, glm::vec<2, int>& left_button_down_window_cusor_pos) noexcept
{
  auto& window  = _windows[handle];
  auto  monitor = Monitor{ handle };
  if (monitor.name() != window.monitor())
  {
    window.set_monitor(monitor.name());
    auto  scale  = monitor.scale();
    auto  ratio  = scale / window.scale();
    if (ratio != 1.f)
    {
      // update left_button_down_window_cursor_pos for next move window is right
      left_button_down_window_cusor_pos = { left_button_down_window_cusor_pos.x * ratio, left_button_down_window_cusor_pos.y * ratio };
      window.resize_by_scale(scale, ratio, cursor_pos, left_button_down_window_cusor_pos);
    }
  }
}

void WindowManager::update_fullscreen_window() noexcept
{
  auto rect = get_virtual_screen_rect();
  if (rect == g_wnd_mgr._fullscreen_window._rect)
    return;

  _fullscreen_window._rect = rect;
  _fullscreen_window.update_by_rect();
  g_renderer.resize_window_resource(_fullscreen_window._handle, _fullscreen_window._width, _fullscreen_window._height);
  SetWindowPos(_fullscreen_window._handle, 0, _fullscreen_window._x, g_wnd_mgr._fullscreen_window._y,
    _fullscreen_window._width, _fullscreen_window._height, SWP_NOZORDER | SWP_NOACTIVATE);
}

auto WindowManager::create_fullscreen_window() noexcept -> HWND
{
  assert(!_fullscreen_window._handle);
  auto rect = get_virtual_screen_rect();
  _fullscreen_window.init_auxiliary(0, 0, rect.right - rect.left, rect.bottom - rect.top);
  return _fullscreen_window._handle;
}

auto WindowManager::create_window(int x, int y, uint32_t width, uint32_t height, ui::Backdrop const& backdrop) noexcept -> HWND
{
  // init window and set handle
  auto window = Window{};
  window.init(x, y, width, height, backdrop);
  
  if (backdrop.style != ui::BackdropStyle::none)
    _blur_windows.emplace(window._blur_window, window._handle);

  auto h = window._handle;

  // store window
  _windows.emplace(window._handle, std::move(window));

  return h;
}

void WindowManager::init_blur_window(HWND handle, ui::Backdrop const& backdrop) noexcept
{
  assert(_windows.contains(handle));
  auto& wnd = _windows[handle];
  wnd.init_blur_window(backdrop);
  _blur_windows.emplace(wnd._blur_window, wnd._handle);
}

void WindowManager::update_blur_window(HWND handle, ui::Backdrop const& backdrop) noexcept
{
  assert(_windows.contains(handle));
  _windows[handle].update_blur_window(backdrop);
}

void WindowManager::close_window(HWND handle) noexcept
{
  assert(_windows.contains(handle));
  _windows[handle].destroy();
  if (auto blur_window = _windows[handle]._blur_window)
    _blur_windows.erase(blur_window);
  _windows.erase(handle);
  _using_mouse_pass_through_windows.erase(handle);
  _window_change_size.erase(handle);
}

void WindowManager::close_fullscreen_window() const noexcept
{
  assert(_fullscreen_window._handle);
  _fullscreen_window.destroy();
}

void WindowManager::destroy_window(HWND handle, HWND blur_handle) const noexcept
{
  if (blur_handle)
    DestroyWindow(blur_handle);
  DestroyWindow(handle);
}

void WindowManager::remove_blur_window(HWND handle) noexcept
{
  assert(_windows.contains(handle));
  auto& wnd = _windows[handle];
  wnd.remove_blur_window();
  _blur_windows.erase(wnd._blur_window);
}

void WindowManager::message_process(MSG const& msg) noexcept
{
  if (msg.message == WM_TIMER)
  {
    if (msg.wParam == _timer_mouse_pass_through)
    {
      // update window mouse pass through state
      for (auto it = _using_mouse_pass_through_windows.begin(); it != _using_mouse_pass_through_windows.end();)
      {
        auto handle = *it;
        if (!_windows[handle].is_mouse_pass_through_area())
        {
          SetWindowLongPtrA(handle, GWL_EXSTYLE, GetWindowLong(handle, GWL_EXSTYLE) & ~(WS_EX_TRANSPARENT | WS_EX_LAYERED));
          it = _using_mouse_pass_through_windows.erase(it);
        }
        else
          ++it;
      }
      if (_using_mouse_pass_through_windows.empty())
      {
        KillTimer(nullptr, _timer_mouse_pass_through);
        _timer_mouse_pass_through = {};
      }
    }
  }

  // send update message to ui context
  g_ui_ctx.cursor_on_window = get_cursor_on_window();
}

auto WindowManager::get_window_z_orders() const noexcept -> std::vector<HWND>
{
  auto handles = std::vector<HWND>();
  handles.reserve(_windows.size());

  auto top = GetTopWindow(nullptr);
  while (top)
  {
    if (_windows.contains(top)) handles.emplace_back(top);
    top = GetWindow(top, GW_HWNDNEXT);
  }

  return handles;
}

auto WindowManager::get_cursor_on_window() noexcept -> HWND
{
  auto z_orders   = g_wnd_mgr.get_window_z_orders();
  auto cursor_pos = get_cursor_pos();
  if (auto it = std::ranges::find_if(z_orders, [&](auto handle) { return _windows[handle].contains_point(cursor_pos); });
      it != z_orders.end())
    return *it;
  return {};
}

}
