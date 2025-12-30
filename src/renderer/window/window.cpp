#include "window.hpp"
#include "../renderer.hpp"
#include "window_manager.hpp"

namespace tk { namespace renderer {

void Window::init(int x, int y, uint32_t width, uint32_t height) noexcept
{
  this->x      = x;
  this->y      = y;
  this->width  = width;
  this->height = height;

  _handle = CreateWindowExW(WS_EX_NOREDIRECTIONBITMAP, WindowManager::Window_Class, nullptr, WS_POPUP | WS_MINIMIZEBOX,
    x, y, width, height, 0, 0, GetModuleHandleW(nullptr), 0);
  err_if(!_handle, "failed to create window");

  // create window render resource
  g_renderer.send_message(Renderer::Message_Window_Create{ _handle, width, height });

  ShowWindow(_handle, SW_SHOW);
}

void Window::destroy() const noexcept
{
  ShowWindow(_handle, SW_HIDE);
  g_renderer.send_message(Renderer::Message_Window_Destroy{ _handle });
}

auto Window::cursor_pos() const noexcept -> glm::vec<2, int>
{
  auto pos = get_cursor_pos();
  return { pos.x - x, pos.y - y };
}

auto Window::is_cursor_valid_area() const noexcept -> bool
{
  // TODO:
  return true;
}

auto Window::contains_point(glm::vec<2, int> p) const noexcept -> bool
{
  return p.x >= x && p.y >= y && p.x <= x + width && p.y <= y + height;
}

}}
