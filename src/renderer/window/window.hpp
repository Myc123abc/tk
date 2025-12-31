#pragma once

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

  void destroy() const noexcept;

  auto contains_point(glm::vec<2, int> p) const noexcept -> bool;

  auto handle() const noexcept { return _handle; }

  auto cursor_pos() const noexcept -> glm::vec<2, int>;

  auto is_cursor_valid_area()  const noexcept -> bool;
  auto is_active() const noexcept { return GetForegroundWindow() == _handle; }

  auto is_moving() const noexcept { return _moving; }
  auto is_resizing() const noexcept { return _resizing; }
  auto is_moving_or_resizing() const noexcept { return _moving || _resizing; }
  void moving_with_pos(int x, int y) noexcept;
  void moving_end() noexcept;

private:
  HWND     _handle{};
  bool     _moving{};
  bool     _resizing{};
public:
  int      x{};
  int      y{};
  uint32_t width{};
  uint32_t height{};
};

}}
