#pragma once

#include <windows.h>

#include <stdint.h>

namespace tk { namespace renderer {

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

private:
  HWND _handle{};
};

}}
