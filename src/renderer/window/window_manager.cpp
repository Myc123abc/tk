#include "window_manager.hpp"
#include "util/error_handling.hpp"
#include "../../ui/ui_context.hpp"

#include <shellscalingapi.h>

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

struct WindowCreateInfo
{
  HANDLE   event{};
  HWND     handle{};
  int      x{}, y{};
  uint32_t width{}, height{};
};

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

inline auto get_monitor_scale(HMONITOR monitor) noexcept
{
  uint32_t dpi_x, dpi_y;
  GetDpiForMonitor(monitor, MDT_EFFECTIVE_DPI, &dpi_x, &dpi_y);
  assert(dpi_x == dpi_y);
  return dpi_x / 96.f;
}

}

namespace tk { namespace renderer {

void WindowManager::init() noexcept
{
  _thread = std::jthread([this]
  {
    _thread_id = GetCurrentThreadId();

    // enable DPI awareness
    SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
    update_monitors();

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

    // create message queue, avoid the first PostMessage failed because message queue is unexit
    PeekMessageW(nullptr, nullptr, 0, 0, PM_NOREMOVE);
    _message_queue_create_complete.count_down();

    _signal_event = CreateEventW(nullptr, false, false, nullptr);

    // message loop
    MSG msg{};
    while (GetMessageW(&msg, nullptr, 0, 0))
    {
      if (msg.message == WM_QUIT) break;
      TranslateMessage(&msg);
      DispatchMessageW(&msg);
      message_process(msg.hwnd, static_cast<Message>(msg.message), msg.wParam, msg.lParam);
      update();
    }
  });
}

void WindowManager::wait_event_process_complete() const noexcept
{
  PostThreadMessageW(_thread_id, std::bit_cast<UINT>(Message::signal), 0, 0);
  WaitForSingleObject(_signal_event, INFINITE);
}

void WindowManager::destroy() noexcept
{
  PostThreadMessageW(_thread_id, WM_QUIT, 0, 0);
  _thread.join();
  CloseHandle(_signal_event);
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

    auto& window = windows.at(handle);
    if (window.is_moving())
      window.move_end();
    else if (window.is_resizing())
      window.resize_end();
  };

  switch (msg)
  {
  case WM_CLOSE:
  {
    g_ui_ctx.send_message(UIContext::Message_Window_Close{ handle });
    return 0;
  }

  case WM_SETTINGCHANGE:
  {
    if (w_param == SPI_SETWORKAREA)
      wnd_mgr._update_monitors = true;
    return 0;
  }

  // when another monitor remove or add, update window position
  case WM_DISPLAYCHANGE:
  {
    wnd_mgr._update_monitors = true;
    if (windows.contains(handle))
    {
      auto& window = windows.at(handle);
      window.monitor_change();
      auto monitor = get_monitor(handle);
      if (monitor != wnd_mgr._window_monitors.at(handle))
        wnd_mgr._window_monitors.at(handle) = monitor;
    }
    return 0;
  }

  case WM_LBUTTONDOWN:
  {
    SetCapture(handle);
    auto& window = windows.at(handle);
    last_cursor_pos                    = get_cursor_pos();
    left_button_down_window_cursor_pos = window.cursor_pos();

    // only moving in cursor valid areas
    auto pt = POINT{ left_button_down_window_cursor_pos.x, left_button_down_window_cursor_pos.y };
    if (!window._move_from_maximize &&
      std::ranges::any_of(g_ui_ctx.access_move_invalid_areas(handle), [pt](auto rect) { return PtInRect(&rect, pt); }))
      break;

    left_button_down_resize_type = window.get_resize_type(left_button_down_window_cursor_pos);
    left_button_down             = true;
    break;
  }

  case WM_MOUSEMOVE:
  {
    auto& window     = wnd_mgr._windows.at(handle);
    auto  cursor_pos = window.cursor_pos();

    // update cursor
    if (auto type = window.get_resize_type(cursor_pos); type != last_resize_type)
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
        if (window.is_maximized())
        {
          window.move_from_maximize();
          left_button_down_window_cursor_pos = window.cursor_pos();
        }
        else
        {
          pos -= left_button_down_window_cursor_pos;
          window.move_with_pos(pos.x, pos.y);
          // resize window when moving window between different scale of monitors
          wnd_mgr.update_monitor(handle, cursor_pos.x, cursor_pos.y);
        }
      }
      // resizing
      else
      {
        auto offset = pos - last_cursor_pos;
        window.adjust_offset(left_button_down_resize_type, pos, offset.x, offset.y);
        window.resize(left_button_down_resize_type, offset.x, offset.y);
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
      g_ui_ctx.send_message(UIContext::Message_Interruption{});
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
    update_monitors();
  }
}

auto CALLBACK WindowManager::enum_display_monitors(HMONITOR monitor, HDC, LPRECT, LPARAM) -> BOOL
{
  auto& info = g_wnd_mgr._monitor_infos[monitor];

  auto monitor_info = MONITORINFO{};
  monitor_info.cbSize = sizeof(monitor_info);
  GetMonitorInfo(monitor, &monitor_info);

  info.rect  = monitor_info.rcMonitor;
  info.scale = get_monitor_scale(monitor);

  return TRUE;
}

void WindowManager::update_monitors() noexcept
{
  _monitor_infos.clear();
  EnumDisplayMonitors(nullptr, nullptr, enum_display_monitors, 0);

  // resize window and update scale of window to ui context if monitor scale is changed
  for (auto& [handle, window] : _windows)
  {
    if (auto monitor = get_monitor(handle))
    {
      auto scale = _monitor_infos.at(monitor).scale;
      if (scale != window.scale())
        window.resize_by_scale(scale);
    }
  }
}

void WindowManager::update_monitor(HWND handle, int x, int y) noexcept
{
  auto monitor = get_monitor(handle);
  if (monitor != _window_monitors.at(handle))
  {
    _window_monitors.at(handle) = monitor;
    if (monitor)
    {
      auto  scale  = _monitor_infos.at(monitor).scale;
      auto& window = _windows.at(handle);
      if (scale != window.scale())
        window.resize_by_scale(scale, x, y);
    }
  }
}

auto WindowManager::create_fullscreen_window() noexcept -> WindowSnapshot
{
  // promise window message queue is built in windows os
  _message_queue_create_complete.wait();

  // create event for wait render resource create complete
  auto event = CreateEventW(nullptr, false, false, nullptr);

  // send window create message to window thread
  PostThreadMessageW(_thread_id, static_cast<UINT>(Message::create_fullscreen_window), std::bit_cast<WPARAM>(event), 0);

  // wait create window complete
  WaitForSingleObject(event, INFINITE);
  CloseHandle(event);
  auto snap = WindowSnapshot{};
  snap.init(_fullscreen_window);
  return snap;
}

auto WindowManager::create_window(int x, int y, uint32_t width, uint32_t height) noexcept -> WindowSnapshot
{
  // create WindowCreateInfo
  auto ptr = reinterpret_cast<WindowCreateInfo*>(malloc(sizeof(WindowCreateInfo)));
  ptr->event  = CreateEventW(nullptr, false, false, nullptr);
  ptr->x      = x;
  ptr->y      = y;
  ptr->width  = width;
  ptr->height = height;

  // send window create message to window thread
  PostThreadMessageW(_thread_id, static_cast<UINT>(Message::create_window), std::bit_cast<WPARAM>(ptr), 0);

  // wait create window complete
  WaitForSingleObject(ptr->event, INFINITE);
  CloseHandle(ptr->event);
  auto snap = WindowSnapshot{};
  snap.init(_windows.at(ptr->handle));
  free(ptr);
  return snap;
}

void WindowManager::close_window(HWND handle) const noexcept
{
  PostThreadMessageW(_thread_id, static_cast<UINT>(Message::close_window), std::bit_cast<WPARAM>(handle), {});
}

void WindowManager::close_fullscreen_window() const noexcept
{
  PostThreadMessageW(_thread_id, static_cast<UINT>(Message::close_fullscreen_window), {}, {});
}

void WindowManager::minimize_window(HWND handle) const noexcept
{
  PostThreadMessageW(_thread_id, static_cast<UINT>(Message::minimize_window), std::bit_cast<WPARAM>(handle), 0);
}

void WindowManager::maximize_window(HWND handle) const noexcept
{
  PostThreadMessageW(_thread_id, static_cast<UINT>(Message::maximize_window), std::bit_cast<WPARAM>(handle), 0);
}

void WindowManager::restore_window(HWND handle) const noexcept
{
  PostThreadMessageW(_thread_id, static_cast<UINT>(Message::restore_window), std::bit_cast<WPARAM>(handle), 0);
}

void WindowManager::message_process(HWND handle, Message msg, WPARAM w_param, LPARAM l_param) noexcept
{
  switch (msg)
  {
  case Message::create_fullscreen_window:
  {
    auto rect = get_virtual_screen_rect();
    _fullscreen_window.init_auxiliary(0, 0, rect.right - rect.left, rect.bottom - rect.top);
    SetEvent(std::bit_cast<HANDLE>(w_param));
    break;
  }

  case Message::create_window:
  {
    msg_create_window(w_param);
    break;
  }

  case Message::signal:
  {
    SetEvent(_signal_event);
    break;
  }

  case Message::close_window:
  {
    auto handle = std::bit_cast<HWND>(w_param);
    _windows.at(handle).destroy();
    _windows.erase(handle);
    _window_monitors.erase(handle);
    _using_mouse_pass_through_windows.erase(handle);
    break;
  }

  case Message::close_fullscreen_window:
  {
    _fullscreen_window.destroy();
    break;
  }

  case Message::minimize_window:
  {
    ShowWindow(std::bit_cast<HWND>(w_param), SW_MINIMIZE);
    break;
  }

  case Message::maximize_window:
  {
    _windows.at(std::bit_cast<HWND>(w_param)).maximize();
    break;
  }

  case Message::restore_window:
  {
    _windows.at(std::bit_cast<HWND>(w_param)).restore();
    break;
  }
  }
  
  if (static_cast<UINT>(msg) == WM_TIMER)
  {
    if (w_param == _timer_mouse_pass_through)
    {
      // update window mouse pass through state
      for (auto it = _using_mouse_pass_through_windows.begin(); it != _using_mouse_pass_through_windows.end();)
      {
        auto handle = *it;
        if (!_windows.at(handle).is_mouse_pass_through_area())
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
  g_ui_ctx.send_message(UIContext::Message_Cursor_On_Window{ get_cursor_on_window() });
}

void WindowManager::msg_create_window(WPARAM w_param) noexcept
{
  // get create info
  auto info = reinterpret_cast<WindowCreateInfo*>(w_param);

  // get scale of monitor
  auto scale   = 1.f;
  auto monitor = HMONITOR{};
  auto infos = _monitor_infos | std::views::values;
  if (auto it = std::ranges::find_if(infos, [x = info->x, y = info->y](auto const& info) { return PtInRect(&info.rect, { x, y }); });
     it != infos.end())
  {
    scale   = it.base()->second.scale;
    monitor = it.base()->first;
  }

  // init window and set handle
  auto window = Window{};
  window.init(info->x, info->y, info->width, info->height, scale);
  info->handle = window._handle;

  // store window
  _windows.emplace(window._handle, std::move(window));
  _window_monitors.emplace(window._handle, monitor);

  // notice window create complete
  SetEvent(info->event);
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

auto WindowManager::get_cursor_on_window() const noexcept -> HWND
{
  auto z_orders   = g_wnd_mgr.get_window_z_orders();
  auto cursor_pos = get_cursor_pos();
  if (auto it = std::ranges::find_if(z_orders, [&](auto handle) { return _windows.at(handle).contains_point(cursor_pos); });
      it != z_orders.end())
    return *it;
  return {};
}

}}
