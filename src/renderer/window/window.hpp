#pragma once

#include "../resource/render_data.hpp"

#include <glm/glm.hpp>

#include <windows.h>

#include <stdint.h>

namespace tk { namespace renderer {

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

  void init(int x, int y, uint32_t width, uint32_t height) noexcept;
  void destroy(RenderDataHandle* ptr) const noexcept;

  auto real_x()      const noexcept { return x      - Window_Shadow_Thickness;     }
  auto real_y()      const noexcept { return y      - Window_Shadow_Thickness;     }
  auto real_width()  const noexcept { return width  + Window_Shadow_Thickness * 2; }
  auto real_height() const noexcept { return height + Window_Shadow_Thickness * 2; }
  auto real_rect()   const noexcept { return RECT{ _rect.left   - static_cast<LONG>(Window_Shadow_Thickness),
                                                   _rect.top    - static_cast<LONG>(Window_Shadow_Thickness),
                                                   _rect.right  + static_cast<LONG>(Window_Shadow_Thickness),
                                                   _rect.bottom + static_cast<LONG>(Window_Shadow_Thickness) }; }

  auto contains_point(glm::vec<2, int> p) const noexcept -> bool;

  auto handle() const noexcept { return _handle; }
  auto pos() const noexcept { return glm::vec<2, int>{ x, y }; }
  auto cursor_pos() const noexcept -> glm::vec<2, int>;

  auto is_cursor_valid_area()  const noexcept -> bool;
  auto is_active() const noexcept { return GetForegroundWindow() == _handle; }
  auto is_mouse_pass_through_area() const noexcept -> bool;

  auto is_moving() const noexcept { return _moving; }
  auto is_resizing() const noexcept { return _resizing; }
  auto is_moving_or_resizing() const noexcept { return _moving || _resizing; }
  void moving_with_pos(int x, int y) noexcept;
  void moving_from_maximize(int x, int y) noexcept;
  void moving_end() noexcept;

  void maximize() noexcept;
  void restore() noexcept;

private:
  void update_by_rect() noexcept;
  void update_rect() noexcept;

public:
  int      x{};
  int      y{};
  uint32_t width{};
  uint32_t height{};
  bool     maximized{};

private:
  HWND     _handle{};
  bool     _moving{};
  bool     _moving_from_maximize{};
  bool     _resizing{};
  RECT     _rect{};
  RECT     _backup_rect{};
};

}}
