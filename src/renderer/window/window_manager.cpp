#include "window_manager.hpp"
#include "../../util/error_handling.hpp"

#include <algorithm>
#include <ranges>

namespace {

auto to_64_bits(uint32_t x, uint32_t y) noexcept
{
  return static_cast<uint64_t>(x) << 32 | y;
}

auto to_32_bits(uint64_t x) noexcept -> std::pair<uint32_t, uint32_t>
{
  return { static_cast<uint32_t>(x >> 32), static_cast<uint32_t>(x & 0xffffffff) };
}

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
    wnd_class.lpszClassName = Fullscreen_Class;
    wnd_class.lpfnWndProc   = DefWindowProcW;
    err_if(!RegisterClassExW(&wnd_class), "failed register class");
    wnd_class.lpszClassName = Window_Class;
    wnd_class.lpfnWndProc   = wnd_proc;
    err_if(!RegisterClassExW(&wnd_class), "failed register class");

    // TODO: create fullscreen window

    // create message queue, avoid the first PostMessage failed because message queue is unexit
    PeekMessageW(nullptr, nullptr, 0, 0, PM_NOREMOVE);
    _message_queue_create_complete.count_down();

    // message loop
    MSG msg{};
    while (GetMessageW(&msg, nullptr, 0, 0))
    {
      if (msg.message == WM_QUIT) return;
      message_process(msg.hwnd, static_cast<Message>(msg.message), msg.wParam, msg.lParam);
      TranslateMessage(&msg);
      DispatchMessageW(&msg);
    }
  });
}

void WindowManager::destroy() noexcept
{
  PostThreadMessageW(_thread_id, static_cast<UINT>(Message::destroy), 0, 0);
  _thread.join();
}

LRESULT CALLBACK WindowManager::wnd_proc(HWND handle, UINT msg, WPARAM w_param, LPARAM l_param) noexcept
{
  static auto& wnd_mgr = WindowManager::instance();

  switch (msg)
  {
  case WM_CLOSE:
  {
    wnd_mgr.close_window(handle);
    return 0;
  }
  }

  return DefWindowProcW(handle, msg, w_param, l_param);
}

void WindowManager::create_window(int x, int y, uint32_t width, uint32_t height) noexcept
{
  _message_queue_create_complete.wait();
  PostThreadMessageW(_thread_id, static_cast<UINT>(Message::create_window), to_64_bits(x, y), to_64_bits(width, height));
}

void WindowManager::message_process(HWND handle, Message msg, WPARAM w_param, LPARAM l_param) noexcept
{
  switch (msg)
  {
  case Message::destroy:
  {
    msg_destroy();
    break;
  }

  case Message::create_window:
  {
    auto [x, y] = to_32_bits(w_param);
    auto [w, h] = to_32_bits(l_param);
    msg_create_window(x, y, w, h);
    break;
  }
  }
}

void WindowManager::msg_destroy() noexcept
{
  std::ranges::for_each(_windows | std::views::values, [](auto const& window) { window.destroy(); });
  PostThreadMessageW(_thread_id, WM_QUIT, 0, 0);
}

void WindowManager::msg_create_window(int x, int y, uint32_t width, uint32_t height) noexcept
{
  auto window = Window{};
  window.init(x, y, width, height);
  _windows.emplace(window._handle, std::move(window));
}

void WindowManager::close_window(HWND handle) noexcept
{
  _windows[handle].destroy();
  _windows.erase(handle);
}

}}
