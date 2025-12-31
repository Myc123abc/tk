#include "window_manager.hpp"
#include "../../util/error_handling.hpp"
#include "../../ui/ui_context.hpp"

using namespace tk::ui;
using namespace tk::window;

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

}

namespace tk { namespace renderer {

void WindowManager::init() noexcept
{
  _thread = std::jthread([this]
  {
    _thread_id = GetCurrentThreadId();

    // register window class
    auto wnd_class = WNDCLASSEXW{};
    wnd_class.cbSize        = sizeof(wnd_class);
    wnd_class.hInstance     = GetModuleHandleW(nullptr);
    wnd_class.hCursor       = LoadCursorA(nullptr, IDC_ARROW);
    wnd_class.lpszClassName = Window_Class;
    wnd_class.lpfnWndProc   = wnd_proc;
    err_if(!RegisterClassExW(&wnd_class), "failed register class");

    // create message queue, avoid the first PostMessage failed because message queue is unexit
    PeekMessageW(nullptr, nullptr, 0, 0, PM_NOREMOVE);
    _message_queue_create_complete.count_down();

    // message loop
    MSG msg{};
    while (GetMessageW(&msg, nullptr, 0, 0))
    {
      if (msg.message == WM_QUIT) return;
      TranslateMessage(&msg);
      DispatchMessageW(&msg);
      message_process(msg.hwnd, static_cast<Message>(msg.message), msg.wParam, msg.lParam);
    }
  });
}

void WindowManager::destroy() noexcept
{
  PostThreadMessageW(_thread_id, WM_QUIT, 0, 0);
  _thread.join();
}

LRESULT CALLBACK WindowManager::wnd_proc(HWND handle, UINT msg, WPARAM w_param, LPARAM l_param) noexcept
{
  static auto& wnd_mgr = WindowManager::instance();
  static auto& windows = wnd_mgr._windows;

  static auto last_cursor_pos                   = glm::vec<2, int>{};
  static auto window_left_button_down_mouse_pos = std::optional<glm::vec<2, int>>{};

  static auto finish_moving_or_resizing = [&](HWND handle)
  {
    ReleaseCapture();
    window_left_button_down_mouse_pos = {};

    auto& window  = windows.at(handle);
    if (window.is_moving())
      window.moving_end();

    // transform new mouse state
    wnd_mgr._mouse_state = MouseState::left_button_up;
    g_ui_ctx.send_message(UIContext::Message_Update_Mouse_State{ wnd_mgr._mouse_state });
    KillTimer(nullptr, wnd_mgr._timer_mouse_state);
    PostMessageW(handle, static_cast<int>(Message::mouse_idle), 0, 0);
  };

  switch (msg)
  {
  case WM_CLOSE:
  {
    g_ui_ctx.send_message(UIContext::Message_Window_Close{ handle });
    return 0;
  }

  case WM_LBUTTONDOWN:
  {
    SetCapture(handle);
    window_left_button_down_mouse_pos = windows.at(handle).cursor_pos();
    assert(window_left_button_down_mouse_pos->x >= 0 && window_left_button_down_mouse_pos->y >= 0);

    wnd_mgr._mouse_state           = MouseState::left_button_down;
    wnd_mgr._mouse_left_down_start = std::chrono::steady_clock::now();
    g_ui_ctx.send_message(UIContext::Message_Update_Mouse_State{ wnd_mgr._mouse_state });
    wnd_mgr._timer_mouse_state = SetTimer(nullptr, 0, USER_TIMER_MINIMUM, nullptr);
    break;
  }

  case WM_MOUSEMOVE:
  {
    auto& window = wnd_mgr._windows.at(handle);

    if (window_left_button_down_mouse_pos)
    {
      auto pos = get_cursor_pos();
      window.moving_with_pos(pos.x - window_left_button_down_mouse_pos->x, pos.y - window_left_button_down_mouse_pos->y);
      SetWindowPos(handle, 0, window.x, window.y, 0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
    }
    
    last_cursor_pos = get_cursor_pos();
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
      finish_moving_or_resizing(handle);
    break;
  }

  }

  return DefWindowProcW(handle, msg, w_param, l_param);
}

auto WindowManager::create_window(int x, int y, uint32_t width, uint32_t height) noexcept -> WindowSnapshot
{
  // promise window message queue is built in windows os
  _message_queue_create_complete.wait();

  // create WindowCreateInfo
  auto ptr = reinterpret_cast<WindowCreateInfo*>(malloc(sizeof(WindowCreateInfo)));
  ptr->event  = CreateEvent(nullptr, false, false, nullptr);
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

void WindowManager::close_window(HWND handle) noexcept
{
  PostThreadMessageW(_thread_id, static_cast<UINT>(Message::close_window), std::bit_cast<WPARAM>(handle), 0);
}

void WindowManager::message_process(HWND handle, Message msg, WPARAM w_param, LPARAM l_param) noexcept
{
  switch (msg)
  {
  case Message::create_window:
  {
    msg_create_window(w_param);
    break;
  }

  case Message::close_window:
  {
    msg_close_window(std::bit_cast<HWND>(w_param));
    break;
  }

  case Message::mouse_idle:
  {
    _mouse_state = MouseState::idle;
    g_ui_ctx.send_message(UIContext::Message_Update_Mouse_State{ _mouse_state });
    break;
  }
  }
  
  if (static_cast<UINT>(msg) == WM_TIMER)
  {
    if (w_param == _timer_mouse_state)
      update_mouse_state();
  }

  for (auto& [handle, window] : _windows)
  {
    // TODO:
    // window.clear_invalid_area();
  }

  // send update message to ui context
  g_ui_ctx.send_message(UIContext::Message_Cursor_On_Window{ get_cursor_on_window() });
}

void WindowManager::update_mouse_state() noexcept
{
  auto dur = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - _mouse_left_down_start).count();
  if (dur > Mouse_Left_Down_Press_Start_Time)
  {
    _mouse_state = MouseState::left_button_press;
    g_ui_ctx.send_message(UIContext::Message_Update_Mouse_State{ _mouse_state });
    KillTimer(nullptr, _timer_mouse_state);
  }
}

void WindowManager::msg_create_window(WPARAM w_param) noexcept
{
  // get create info
  auto info = reinterpret_cast<WindowCreateInfo*>(w_param);

  // init window and set handle
  auto window = Window{};
  window.init(info->x, info->y, info->width, info->height);
  info->handle = window._handle;

  // store window
  _windows.emplace(window._handle, std::move(window));

  // notice window create complete
  SetEvent(info->event);
}

void WindowManager::msg_close_window(HWND handle) noexcept
{
  _windows[handle].destroy();
  _windows.erase(handle);
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
