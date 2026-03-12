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

inline auto point_on(glm::vec<2, int> const& p, glm::vec2 const& left_top, glm::vec2 const& right_bottom) noexcept
{
  return p.x >= left_top.x && p.x <= right_bottom.x && p.y >= left_top.y && p.y <= right_bottom.y;
}

inline auto point_on(glm::vec<2, int> const& p, RECT rect) noexcept
{
  return p.x >= rect.left && p.x <= rect.right && p.y >= rect.top && p.y <= rect.bottom;
}

inline auto operator==(RECT lhs, RECT rhs) -> bool
{
  return EqualRect(&lhs, &rhs);
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
  bool     fullscreen_window{};
  
  void init(Window const& window) noexcept
  {
    handle    = window.handle();
    x         = window.x();
    y         = window.y();
    width     = window.width();
    height    = window.height();
    maximized = window.is_maximized();
    scale     = window.scale();
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
    fullscreen_window,
    restore_fullscreen_window,
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
  void fullscreen_window(HWND handle) const noexcept;
  void restore_fullscreen_window(HWND handle) const noexcept;

  auto get_window_z_orders() const noexcept -> std::vector<HWND>;
  auto get_cursor_on_window() const noexcept -> HWND;

private:
  void message_process(HWND handle, Message msg, WPARAM w_param, LPARAM l_param) noexcept;
  void msg_create_window(WPARAM w_param) noexcept;

  void update() noexcept;
  void update_monitor(HWND handle, glm::vec2 cursor_pos, glm::vec<2, int>& left_button_down_window_cursor_pos) noexcept;
  void update_fullscreen_window() noexcept;

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
  
  bool                             _update_monitors{};
  std::unordered_map<HWND, RECT>   _window_change_size{};
};

inline static auto& g_wnd_mgr{ WindowManager::instance() };

}}
