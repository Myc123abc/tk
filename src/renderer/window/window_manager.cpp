#include "window_manager.hpp"
#include "../../util/error_handling.hpp"
#include "../../ui/ui_context.hpp"

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
    window.moving = false;

    // transform new mouse state
    if (window.mouse_state == MouseState::left_button_down || window.mouse_state == MouseState::left_button_press)
      window.mouse_state = MouseState::left_button_up;
  };

  using namespace ui;

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
    windows.at(handle).mouse_state = MouseState::left_button_down;
    break;
  }

  case WM_MOUSEMOVE:
  {
    auto& window = wnd_mgr._windows.at(handle);

    if (window_left_button_down_mouse_pos)
    {
      auto pos = get_cursor_pos();
      window.x = pos.x - window_left_button_down_mouse_pos->x;
      window.y = pos.y - window_left_button_down_mouse_pos->y;
      SetWindowPos(handle, 0, window.x, window.y, 0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
      window.moving = true;
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
    {
      finish_moving_or_resizing(handle);
    }
    break;
  }

  }

  return DefWindowProcW(handle, msg, w_param, l_param);
}

auto WindowManager::create_window(int x, int y, uint32_t width, uint32_t height) noexcept -> HWND
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
  auto handle = ptr->handle;
  free(ptr);
  return handle;
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

  case Message::left_button_press:
  {
    _windows.at(handle).mouse_state = MouseState::left_button_press;
    break;
  }

  case Message::mouse_idle:
  {
    _windows.at(handle).mouse_state = MouseState::idle;
    break;
  }
  }

  for (auto& [handle, window] : _windows)
  {
    // TODO:
    // window.clear_invalid_area();

    // post next mouse state
    if (window.mouse_state == MouseState::left_button_down)
      PostMessageW(handle, static_cast<int>(Message::left_button_press), 0, 0);
    else if (window.mouse_state == MouseState::left_button_up)
      PostMessageW(handle, static_cast<int>(Message::mouse_idle), 0, 0);
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

}}
