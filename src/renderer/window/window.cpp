#include "window.hpp"
#include "../renderer.hpp"
#include "window_manager.hpp"
#include "../../ui/ui_context.hpp"

using namespace tk::ui;

namespace tk { namespace renderer {

void Window::init(int x, int y, uint32_t width, uint32_t height) noexcept
{
  this->x      = x;
  this->y      = y;
  this->width  = width;
  this->height = height;
  update_rect();

  _handle = CreateWindowExW(WS_EX_NOREDIRECTIONBITMAP, WindowManager::Window_Class, nullptr, WS_POPUP | WS_MINIMIZEBOX,
    x, y, width, height, 0, 0, GetModuleHandleW(nullptr), 0);
  err_if(!_handle, "failed to create window");

  // create window render resource
  g_renderer.send_message(Renderer::Message_Window_Create{ _handle, width, height });

  ShowWindow(_handle, SW_SHOW);
}

void Window::update_by_rect() noexcept
{
  x      = _rect.left;
  y      = _rect.top;
  width  = _rect.right - _rect.left;
  height = _rect.bottom - _rect.top;
}

void Window::update_rect() noexcept
{
  _rect = { x, y, static_cast<LONG>(x + width), static_cast<LONG>(y + height) };
}

void Window::destroy(RenderDataHandle* ptr) const noexcept
{
  ShowWindow(_handle, SW_HIDE);
  g_renderer.send_message(Renderer::Message_Window_Destroy{ _handle, ptr });
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
  return PtInRect(&_rect, { p.x, p.y });
}
static bool log = false;
void Window::moving_from_maximize(int x, int y) noexcept
{
  auto ratio_x = static_cast<float>(x) / width;

  _moving   = true;
  maximized = false;
  width     = _backup_rect.right  - _backup_rect.left;
  height    = _backup_rect.bottom - _backup_rect.top;
  this->x   = x - width * ratio_x;
  this->y   = 0;
  update_rect();
  g_renderer.send_message(Renderer::Message_Window_Update{ _handle, width, height });
  g_ui_ctx.send_message(UIContext::Message_Update_Moving{ _handle, _moving, this->x, this->y });
  g_ui_ctx.send_message(UIContext::Message_Window_Restore{ _handle, this->x, this->y, width, height });
  SetWindowPos(_handle, 0, this->x, this->y, width, height, SWP_NOZORDER | SWP_NOACTIVATE);
};

void Window::moving_with_pos(int x, int y) noexcept
{
  this->x = x;
  this->y = y;
  _moving = true;
  update_rect();
  g_ui_ctx.send_message(UIContext::Message_Update_Moving{ _handle, _moving, x, y });
  SetWindowPos(_handle, 0, x, y, 0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
}

void Window::moving_end() noexcept
{
  _moving = false;
  g_ui_ctx.send_message(UIContext::Message_Update_Moving{ _handle, _moving, x, y });
}

// TODO: send resizing message

void Window::maximize() noexcept
{
  maximized    = true;
  _backup_rect =_rect;
  _rect        = get_maximize_rect();
  update_by_rect();
  g_renderer.send_message(Renderer::Message_Window_Update{ _handle, width, height });
  g_ui_ctx.send_message(UIContext::Message_Window_Maximize{ _handle, x, y, width, height });
  SetWindowPos(_handle, 0, x, y, width, height, SWP_NOZORDER | SWP_NOACTIVATE);
}

void Window::restore() noexcept
{
  maximized = false;
  _rect     = _backup_rect;
  update_by_rect();
  g_renderer.send_message(Renderer::Message_Window_Update{ _handle, width, height });
  g_ui_ctx.send_message(UIContext::Message_Window_Restore{ _handle, x, y, width, height });
  SetWindowPos(_handle, 0, x, y, width, height, SWP_NOZORDER | SWP_NOACTIVATE);
}

}}
