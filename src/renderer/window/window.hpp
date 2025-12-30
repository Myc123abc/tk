#pragma once

#include <glm/glm.hpp>

#include <windows.h>

#include <stdint.h>

namespace tk { namespace renderer {

enum class MouseState
{
  idle,
  left_button_down,
  left_button_press,
  left_button_up,
};

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

  void init(int x, int y, uint32_t width, uint32_t height) noexcept;

  void destroy() const noexcept;

  auto contains_point(glm::vec<2, int> p) const noexcept -> bool;

  auto handle() const noexcept { return _handle; }

  auto cursor_pos() const noexcept -> glm::vec<2, int>;

  auto is_cursor_valid_area()  const noexcept -> bool;
  auto is_moving_or_resizing() const noexcept { return moving || resizing; }
  auto is_active() const noexcept { return GetForegroundWindow() == _handle; }

private:
  HWND _handle{};

public:
  int        x{};
  int        y{};
  uint32_t   width{};
  uint32_t   height{};
  bool       moving{};
  bool       resizing{};
  MouseState mouse_state{};
};

}}
