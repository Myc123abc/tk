#pragma once

#include "../config.hpp"
#include "compositor.hpp"
#include "ui/ui.hpp"

#include "util/vec.hpp"

#include <windows.h>

#include <stdint.h>
#include <string>

namespace tk::renderer {

enum class CursorType
{
  arrow,
  up_down,
  left_rigtht,
  diagonal,
  anti_diagonal
};

enum class ResizeType
{
  none,
  left_top,
  right_top,
  left_bottom,
  right_bottom,
  left,
  right,
  top,
  bottom,
};

class Window
{
  friend class WindowManager;
public:
  Window()                         = default;
  ~Window()                        = default;
  Window(Window const&)            = default;
  Window(Window&&)                 = default;
  Window& operator=(Window const&) = default;
  Window& operator=(Window&&)      = default;

  void init(int x, int y, uint32_t width, uint32_t height, ui::Backdrop const& backdrop) noexcept;
  void init_auxiliary(int x, int y, uint32_t width, uint32_t height) noexcept;
  void destroy() const noexcept;
  
  auto shadow_thickness() const noexcept -> LONG { return Window_Shadow_Thickness * _scale; }
  auto real_x()      const noexcept -> int      { return _x      - shadow_thickness();     }
  auto real_y()      const noexcept -> int      { return _y      - shadow_thickness();     }
  auto real_width()  const noexcept -> uint32_t { return _width  + shadow_thickness() * 2; }
  auto real_height() const noexcept -> uint32_t { return _height + shadow_thickness() * 2; }
  auto real_rect()   const noexcept { return RECT{ _rect.left   - shadow_thickness(),
                                                   _rect.top    - shadow_thickness(),
                                                   _rect.right  + shadow_thickness(),
                                                   _rect.bottom + shadow_thickness() }; }
  auto rect() const noexcept { return _rect; }
  
  auto content_rect() const noexcept { return RECT{ shadow_thickness(),
                                                    shadow_thickness(),
                                                    static_cast<LONG>(_width  + shadow_thickness()),
                                                    static_cast<LONG>(_height + shadow_thickness()) }; }
  auto shadow_rect() const noexcept { return RECT{ 0, 0,
    static_cast<LONG>(_width + shadow_thickness() * 2), static_cast<LONG>(_height + shadow_thickness()) * 2 }; }

  auto contains_point(vec2i p) const noexcept -> bool;

  auto handle() const noexcept { return _handle; }
  auto pos() const noexcept { return vec2i{ _x, _y }; }
  auto real_pos() const noexcept { return vec2 { _x - shadow_thickness(), _y - shadow_thickness() }; }
  auto cursor_pos() const noexcept -> vec2i;
  auto real_cursor_pos() const noexcept -> vec2i;

  auto real_extent() const noexcept { return vec2u{ _width + shadow_thickness() * 2, _height + shadow_thickness() * 2 }; }

  auto is_active() const noexcept { return GetForegroundWindow() == _handle; }
  auto is_mouse_pass_through_area() const noexcept -> bool;

  auto x() const noexcept { return _x; }
  auto y() const noexcept { return _y; }
  auto width() const noexcept { return _width; }
  auto height() const noexcept { return _height; }
  auto scale() const noexcept { return _scale; }

  auto is_fullscreen() const noexcept { return _fullscreen; }
  auto is_maximized() const noexcept { return _maximized; }
  auto is_moving() const noexcept { return _moving; }
  auto is_resizing() const noexcept { return _resizing; }
  auto is_moving_or_resizing() const noexcept { return _moving || _resizing; }
  auto is_move_from_maximize() const noexcept { return _move_from_maximize; }
  void move_with_pos(int x, int y) noexcept;
  auto move_from_maximize() noexcept -> vec2i;
  void move_end() noexcept;
  void adjust_offset(ResizeType type, vec2i point, int& dx, int& dy) const noexcept;
  void resize(ResizeType type, int dx, int dy) noexcept;
  void resize_end() noexcept;
  void resize_by_scale(float scale, float ratio, vec2 cursor_pos, vec2 left_button_down_window_cusor_pos) noexcept;
  void reset_pos_size() noexcept;
  void update_by_real_rect(RECT rect, float scale) noexcept;
  void update_by_rect(RECT rect, float scale) noexcept;

  void maximize() noexcept;
  void minimize() noexcept;
  void cancel_maximize(RECT rect, float scale) noexcept;
  void restore() noexcept;
  void fullscreen() noexcept;
  void restore_fullscreen() noexcept;
  void cancel_fullscreen(RECT rect, float scale) noexcept;
  void cancel_fullscreen_maximize(RECT rect, float scale) noexcept;

  auto get_resize_type(vec2i p) const noexcept -> ResizeType;

  auto monitor() const noexcept { return _monitor; }
  auto set_monitor(std::string monitor) noexcept { _monitor = monitor; }

  void add_move_invalid_area(RECT rect) noexcept { _move_invalid_areas.emplace_back(rect); }
  void clear_move_invalid_areas() noexcept { _move_invalid_areas.clear(); }

  auto& cfg() noexcept { return _cfg; }
  auto& cfg() const noexcept { return _cfg; }

private:
  void update_by_rect() noexcept;
  void update_rect() noexcept;
  auto cursor_valid_area()  const noexcept -> RECT;

  void left_offset(int dx)   noexcept;
  void top_offset(int dy)    noexcept;
  void right_offset(int dx)  noexcept;
  void bottom_offset(int dy) noexcept;
  
  auto min_width() const noexcept { return Window_Min_Width * _scale; }
  auto min_height() const noexcept { return Window_Min_Height * _scale; }

  void init_blur_window(ui::Backdrop const& backdrop) noexcept;
  void update_blur_window(ui::Backdrop const& backdrop) noexcept;
  void keep_blur_window_behind() const noexcept;
  void keep_blur_window_behind_move() const noexcept;
  void keep_blur_window_behind_resize() const noexcept;
  void resize_blur_window(RECT rect) const noexcept;
  void remove_blur_window() noexcept;
public:
  void show_blur_window() const noexcept;

private:
  int                  _x{};
  int                  _y{};
  uint32_t             _width{};
  uint32_t             _height{};
  HWND                 _handle{};
  float                _scale{ 1.f };
  bool                 _fullscreen{};
  bool                 _maximized{};
  bool                 _moving{};
  bool                 _move_from_maximize{};
  bool                 _resizing{};
  RECT                 _rect{};
  RECT                 _backup_rect{};
  std::string          _monitor{};
  std::vector<RECT>    _move_invalid_areas;
  HWND                 _blur_window{};
  Compositor::Resource _blur_res{};
  ui::WindowConfig     _cfg{};
};

}
