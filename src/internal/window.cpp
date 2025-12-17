module;

#include <windows.h>
#include <unordered_map>
#include <ranges>
#include <algorithm>
#include <thread>
#include <latch>
#include <atomic>

module tk:window;

import :error_handling;

namespace {

constexpr wchar_t Fullscreen_Class[] = L"vn::window::WindowManager::Fullscreen";
constexpr wchar_t Window_Class[]     = L"vn::window::WindowManager::Window";

auto to_64_bits(uint32_t x, uint32_t y) noexcept
{
  return static_cast<uint64_t>(x) << 32 | y;
}

auto to_32_bits(uint64_t x) noexcept -> std::pair<uint32_t, uint32_t>
{
  return { x >> 32, x & 0xffffffff };
}

}

namespace tk { namespace window {

////////////////////////////////////////////////////////////////////////////////
///                               Window
////////////////////////////////////////////////////////////////////////////////

class WindowManager;
class Window
{
  friend class WindowManager;
public:
  Window()                         = default;
  ~Window()                        = default;
  Window(Window const&)            = delete;
  Window(Window&&)                 = default;
  Window& operator=(Window const&) = delete;
  Window& operator=(Window&&)      = delete;

  void init(int x, int y, uint32_t width, uint32_t height) noexcept
  {
    // _handle = CreateWindowExW(WS_EX_NOREDIRECTIONBITMAP, Window_Class, nullptr, WS_POPUP | WS_MINIMIZEBOX,
    _handle = CreateWindowExW(WS_EX_NOREDIRECTIONBITMAP, Window_Class, nullptr, WS_OVERLAPPEDWINDOW,
      x, y, width, height, 0, 0, GetModuleHandleW(nullptr), 0);
    err_if(!_handle,  "failed to create window");
    ShowWindow(_handle, SW_SHOW); // TODO: show after first frame render finish and also include some images uploaded finish
  }

  void destroy() const noexcept
  {
    ShowWindow(_handle, SW_HIDE);
    DestroyWindow(_handle);
  }

private:
  HWND _handle{};
};

////////////////////////////////////////////////////////////////////////////////
///                           Window Manager
////////////////////////////////////////////////////////////////////////////////

class WindowManager
{
  enum class Message
  {
    destroy = WM_APP,
    create_window,
  };

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

  void init() noexcept
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

  void destroy() noexcept
  {
    PostThreadMessageW(_thread_id, static_cast<UINT>(Message::destroy), 0, 0);
    _thread.join();
  }

  static LRESULT CALLBACK wnd_proc(HWND handle, UINT msg, WPARAM w_param, LPARAM l_param) noexcept
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

  auto create_window(int x, int y, uint32_t width, uint32_t height) noexcept
  {
    _message_queue_create_complete.wait();
    PostThreadMessageW(_thread_id, static_cast<UINT>(Message::create_window), to_64_bits(x, y), to_64_bits(width, height));
    _window_count.fetch_add(1, std::memory_order_release);
  }

  auto window_count() const noexcept { return _window_count.load(std::memory_order_acquire); }

////////////////////////////////////////////////////////////////////////////////
///                           Message Process
////////////////////////////////////////////////////////////////////////////////

  void message_process(HWND handle, Message msg, WPARAM w_param, LPARAM l_param) noexcept
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

  void msg_destroy() noexcept
  {
    std::ranges::for_each(_windows | std::views::values, [](auto const& window) { window.destroy(); });
    PostThreadMessageW(_thread_id, WM_QUIT, 0, 0);
  }

  void msg_create_window(int x, int y, uint32_t width, uint32_t height) noexcept
  {
    auto window = Window{};
    window.init(x, y, width, height);
    _windows.emplace(window._handle, std::move(window));
  }
  
  void close_window(HWND handle) noexcept
  {
    _windows[handle].destroy();
    _windows.erase(handle);
    _window_count.fetch_sub(1, std::memory_order_release);
  }

private:
  std::jthread                     _thread;
  DWORD                            _thread_id{};
  std::latch                       _message_queue_create_complete{ 1 };
  std::atomic_uint32_t             _window_count{};

  std::unordered_map<HWND, Window> _windows;
};

auto& g_wnd_mgr{ WindowManager::instance() };

}}
