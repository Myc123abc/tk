#pragma once

#include "window.hpp"
#include "window/type.hpp"

#include <thread>
#include <latch>
#include <unordered_map>

namespace tk { namespace renderer {

inline auto get_cursor_pos() noexcept
{
  auto p = POINT{};
  GetCursorPos(&p);
  return glm::vec<2, int>{ p.x, p.y };
}

struct WindowSnapshot
{
  HWND handle{};
  int  x{};
  int  y{};
  bool moving{};
  bool resizing{};
  
  void init(Window const& window) noexcept
  {
    handle = window.handle();
    x      = window.x;
    y      = window.y;
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
    close_window,
    mouse_idle,
  };

  void init() noexcept;

  void destroy() noexcept;

  static LRESULT CALLBACK wnd_proc(HWND handle, UINT msg, WPARAM w_param, LPARAM l_param) noexcept;

  auto create_window(int x, int y, uint32_t width, uint32_t height) noexcept -> WindowSnapshot;
  void close_window(HWND handle) noexcept;

  auto get_window_z_orders() const noexcept -> std::vector<HWND>;
  auto get_cursor_on_window() const noexcept -> HWND;

private:
  void message_process(HWND handle, Message msg, WPARAM w_param, LPARAM l_param) noexcept;

  void msg_create_window(WPARAM w_param) noexcept;
  void msg_close_window(HWND handle) noexcept;

  // mouse state process
  void update_mouse_state() noexcept;
  UINT_PTR                              _timer_mouse_state{};
  window::MouseState                    _mouse_state{};
  std::chrono::steady_clock::time_point _mouse_left_down_start{};

private:
  static constexpr wchar_t Window_Class[] = L"vn::window::WindowManager::Window";

private:
  std::jthread                     _thread;
  DWORD                            _thread_id{};
  std::latch                       _message_queue_create_complete{ 1 };
  std::unordered_map<HWND, Window> _windows;
};

inline static auto& g_wnd_mgr{ WindowManager::instance() };

}}
