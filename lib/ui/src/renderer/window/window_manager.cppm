module;

#include <windows.h>

#include <cassert>
#include <optional>
#include <ranges>
#include <unordered_map>
#include <unordered_set>

export module tk.renderer:window_manager;

import :window;
import tk.singleton;
import tk.base;
import tk.rect;
import tk.ui.window_config;

export namespace tk::renderer {

inline auto get_cursor_pos() noexcept
{
  auto p = POINT{};
  GetCursorPos(&p);
  return int2{ p.x, p.y };
}

class WindowManager : public Singleton<WindowManager>
{
public:
  void init() noexcept;
  void message_process() noexcept;
  void destroy() noexcept;

  auto windows_view() const noexcept { return _windows | std::views::values; }
  auto get_window(HWND handle) noexcept { assert(_windows.contains(handle)); return &_windows[handle]; }

  static LRESULT CALLBACK wnd_proc(HWND handle, UINT msg, WPARAM w_param, LPARAM l_param) noexcept;
  static LRESULT CALLBACK blur_wnd_proc(HWND handle, UINT msg, WPARAM w_param, LPARAM l_param) noexcept;

  auto create_fullscreen_window() noexcept -> HWND;
  auto create_window(std::optional<float2> pos, uint width, uint height, ui::Backdrop const& backdrop) noexcept -> HWND;
  void close_window(HWND handle) noexcept;
  void close_fullscreen_window() const noexcept;
  void destroy_window(HWND handle, HWND blur_handle) const noexcept;
  void init_blur_window(HWND handle, ui::Backdrop const& backdrop) noexcept;
  void update_blur_window(HWND handle, ui::Backdrop const& backdrop) noexcept;
  void remove_blur_window(HWND handle) noexcept;
  void resize_blur_window(HWND handle, Rect rect) noexcept;

  auto get_cursor_on_window() noexcept -> HWND;

  auto is_normal_cursor() const noexcept { return _cursor_type == ResizeType::none; }

private:
  void message_process(MSG const& msg) noexcept;

  void update() noexcept;
  void update_monitor(HWND handle, float2 cursor_pos, int2& left_button_down_window_cursor_pos) noexcept;
  void update_monitors() noexcept;
  void update_fullscreen_window() noexcept;
  void update_cursor() noexcept;

public:
  static constexpr wchar_t Auxiliary_Class[] = L"vn::window::WindowManager::AuxiliaryWindow";
  static constexpr wchar_t Window_Class[]    = L"vn::window::WindowManager::Window";
  static constexpr wchar_t Blur_Class[]      = L"vn::window::WindowManager::BlurWindow";

private:
  Window                           _fullscreen_window;
  std::unordered_map<HWND, Window> _windows;
  std::unordered_set<HWND>         _using_mouse_pass_through_windows;
  UINT_PTR                         _timer_mouse_pass_through{};
  bool                             _update_monitors{};
  std::unordered_map<HWND, Rect>   _window_change_size{};
  std::unordered_map<HWND, HWND>   _blur_windows;
  ResizeType                       _cursor_type{};
  bool                             _resizing{};
};

auto& g_wnd_mgr = WindowManager::instance();

}
