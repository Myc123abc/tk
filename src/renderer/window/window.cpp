#include "window.hpp"
#include "../renderer.hpp"
#include "window_manager.hpp"
#include "../../ui/ui_context.hpp"
#include "../config.hpp"

using namespace tk::ui;

namespace
{

auto in_range(float v, float min, float max) noexcept
{
  return v >= min && v <= max;
}

}

namespace tk { namespace renderer {

void Window::init(int x, int y, uint32_t width, uint32_t height) noexcept
{
  this->x      = x;
  this->y      = y;
  this->width  = width;
  this->height = height;
  update_rect();

  _handle = CreateWindowExW(WS_EX_NOREDIRECTIONBITMAP, WindowManager::Window_Class, nullptr, WS_POPUP | WS_MINIMIZEBOX,
    real_x(), real_y(), real_width(), real_height(), 0, 0, GetModuleHandleW(nullptr), 0);
  err_if(!_handle, "failed to create window");

  // create window render resource
  g_renderer.send_message(Renderer::Message_Window_Create{ _handle, real_width(), real_height() });

  ShowWindow(_handle, SW_SHOW);
}

void Window::init_auxiliary(int x, int y, uint32_t width, uint32_t height) noexcept
{
  this->x      = x;
  this->y      = y;
  this->width  = width;
  this->height = height;
  update_rect();

  _handle = CreateWindowExW(WS_EX_NOREDIRECTIONBITMAP | WS_EX_LAYERED | WS_EX_TRANSPARENT | WS_EX_TOOLWINDOW | WS_EX_TOPMOST,
    WindowManager::Auxiliary_Class, nullptr, WS_POPUP, x, y, width, height, 0, 0, GetModuleHandleW(nullptr), 0);
  err_if(!_handle, "failed to create window");

  // create window render resource
  g_renderer.send_message(Renderer::Message_Window_Create{ _handle, real_width(), real_height() });

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

auto Window::cursor_valid_area() const noexcept -> RECT
{
  auto rect =_rect;
  rect.left   -= Window_Resize_Thickness;
  rect.top    -= Window_Resize_Thickness;
  rect.right  += Window_Resize_Thickness;
  rect.bottom += Window_Resize_Thickness;
  return rect;
}

auto Window::is_mouse_pass_through_area() const noexcept -> bool
{
  auto rect = cursor_valid_area();
  auto pos  = get_cursor_pos();
  return !PtInRect(&rect, { pos.x, pos.y });
}

auto Window::contains_point(glm::vec<2, int> p) const noexcept -> bool
{
  return PtInRect(&_rect, { p.x, p.y });
}

void Window::move_from_maximize(int x, int y) noexcept
{
  auto ratio_x = static_cast<float>(x) / width;

  _move_from_maximize = true;
  _moving   = true;
  maximized = false;
  width     = _backup_rect.right  - _backup_rect.left;
  height    = _backup_rect.bottom - _backup_rect.top;
  this->x   = x - width * ratio_x;
  this->y   = y < Window_Y_Pos_Moving_From_Maximize ? -Window_Y_Pos_Moving_From_Maximize : 0;
  update_rect();
  g_renderer.send_message(Renderer::Message_Window_Update{ _handle, real_width(), real_height() });
  g_ui_ctx.send_message(UIContext::Message_Window_Moving_From_Maximize{ _handle, this->x, this->y, width, height });
  SetWindowPos(_handle, 0, real_x(), real_y(), real_width(), real_height(), SWP_NOZORDER | SWP_NOACTIVATE);
}

void Window::move_with_pos(int x, int y) noexcept
{
  this->x = x;
  this->y = y;
  _moving = true;
  update_rect();
  g_ui_ctx.send_message(UIContext::Message_Update_Moving{ _handle, _moving, x, y });
  SetWindowPos(_handle, 0, real_x(), real_y(), 0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
}

void Window::move_end() noexcept
{
  _moving = false;
  if (_move_from_maximize)
  {
    _move_from_maximize = false;
    g_ui_ctx.send_message(UIContext::Message_Window_Moving_From_Maximize_End{ _handle, x, y });
  }
  else
    g_ui_ctx.send_message(UIContext::Message_Update_Moving{ _handle, _moving, x, y });
}

void Window::adjust_offset(ResizeType type, glm::vec<2, int> const& point, int& dx, int& dy) const noexcept
{
  using enum ResizeType;
  switch (type)
  {
  case none:
    break;

  case left_top:
    if (width  == _min_width  && point.x > _rect.left) dx = 0;
    if (height == _min_height && point.y > _rect.top)  dy = 0;
    break;

  case right_top:
    if (width  == _min_width  && point.x < _rect.right) dx = 0;
    if (height == _min_height && point.y > _rect.top)   dy = 0;
    break;

  case left_bottom:
    if (width  == _min_width  && point.x > _rect.left)   dx = 0;
    if (height == _min_height && point.y < _rect.bottom) dy = 0;
    break;

  case right_bottom:
    if (width  == _min_width  && point.x < _rect.right)  dx = 0;
    if (height == _min_height && point.y < _rect.bottom) dy = 0;
    break;

  case left:
    if (width == _min_width && point.x > _rect.left) dx = 0;
    break;

  case right:
    if (width == _min_width && point.x < _rect.right) dx = 0;
    break;

  case top:
    if (height == _min_height && point.y > _rect.top) dy = 0;
    break;

  case bottom:
    if (height == _min_height && point.y < _rect.bottom) dy = 0;
    break;
  }
}

void Window::resize(ResizeType type, int dx, int dy) noexcept
{
  _resizing = true;

  using enum ResizeType;
  switch (type)
  {
  case none:
    return;

  case left_top:
    left_offset(dx);
    top_offset(dy);
    break;

  case right_top:
    right_offset(dx);
    top_offset(dy);
    break;

  case left_bottom:
    left_offset(dx);
    bottom_offset(dy);
    break;

  case right_bottom:
    right_offset(dx);
    bottom_offset(dy);
    break;

  case left:
    left_offset(dx);
    break;

  case right:
    right_offset(dx);
    break;

  case top:
    top_offset(dy);
    break;

  case bottom:
    bottom_offset(dy);
    break;
  }
  update_by_rect();

  g_ui_ctx.send_message(UIContext::Message_Update_Resizing{ _handle, x, y, width, height });
}

void Window::resize_end() noexcept
{
  _resizing = false;
  g_ui_ctx.send_message(UIContext::Message_Resize_End{ _handle });
  g_renderer.send_message(Renderer::Message_Window_Update{ _handle, real_width(), real_height() });
  SetWindowPos(_handle, 0, real_x(), real_y(), real_width(), real_height(), SWP_NOZORDER | SWP_NOACTIVATE);
}

void Window::left_offset(int dx) noexcept
{
  auto rc = get_maximize_rect();
  _rect.left = std::clamp(
    _rect.left + dx,
    rc.left,
    static_cast<LONG>(std::min(_rect.right, rc.right) - _min_width)
  );
  auto p = get_cursor_pos();
  if (dx < 0 && _rect.left < p.x && rc.right - _rect.left > _min_width) _rect.left = p.x;
  if (_rect.right > rc.right && rc.right - _rect.left < _min_width) _rect.left = rc.right - _min_width;
}

void Window::top_offset(int dy) noexcept
{
  auto rc = get_maximize_rect();
  _rect.top = std::clamp(
    _rect.top + dy,
    rc.top,
    static_cast<LONG>(std::min(_rect.bottom, rc.bottom) - _min_height)
  );
  auto p = get_cursor_pos();
  if (dy < 0 && _rect.top < p.y && rc.bottom - _rect.top > _min_height) _rect.top = p.y;
  if (_rect.bottom > rc.bottom && rc.bottom - _rect.top < _min_height) _rect.top = rc.bottom - _min_width;
}

void Window::right_offset(int dx) noexcept
{
  auto rc = get_maximize_rect();
  _rect.right = std::clamp(
    _rect.right + dx,
    static_cast<LONG>(std::max(_rect.left, rc.left) + _min_width),
    rc.right
  );
  auto p = get_cursor_pos();
  if (dx > 0 && _rect.right > p.x && _rect.right - rc.left > _min_width) _rect.right = p.x;
  if (_rect.left < rc.left && _rect.right - rc.left < _min_width) _rect.right = rc.left + _min_width;
  if (_rect.right == rc.right - 1) _rect.right = rc.right; // I don't know why in this case the dx is always 0
}

void Window::bottom_offset(int dy) noexcept
{
  auto rc = get_maximize_rect();
  _rect.bottom = std::clamp(
    _rect.bottom + dy,
    static_cast<LONG>(std::max(_rect.top, rc.top) + _min_height),
    rc.bottom
  );
  auto p = get_cursor_pos();
  if (dy > 0 && _rect.bottom > p.y && _rect.bottom - rc.top > _min_height) _rect.bottom = p.y;
  if (_rect.top < rc.top && _rect.bottom - rc.top < _min_height) _rect.bottom = rc.top + _min_height;
  if (_rect.bottom == rc.bottom - 1) _rect.bottom = rc.bottom; // I don't know why in this case the dy is always 0
}

void Window::maximize() noexcept
{
  maximized    = true;
  _backup_rect =_rect;
  _rect        = get_maximize_rect();
  update_by_rect();
  g_renderer.send_message(Renderer::Message_Window_Update{ _handle, real_width(), real_height() });
  g_ui_ctx.send_message(UIContext::Message_Window_Maximize{ _handle, x, y, width, height });
  SetWindowPos(_handle, 0, real_x(), real_y(), real_width(), real_height(), SWP_NOZORDER | SWP_NOACTIVATE);
}

void Window::restore() noexcept
{
  maximized = false;
  _rect     = _backup_rect;
  update_by_rect();
  g_renderer.send_message(Renderer::Message_Window_Update{ _handle, real_width(), real_height() });
  g_ui_ctx.send_message(UIContext::Message_Window_Restore{ _handle, x, y, width, height });
  SetWindowPos(_handle, 0, real_x(), real_y(), real_width(), real_height(), SWP_NOZORDER | SWP_NOACTIVATE);
}

auto Window::get_resize_type(glm::vec<2, int> const& p) const noexcept -> ResizeType
{
  using enum ResizeType;

  if (maximized) return none;

  auto left_side   = in_range(p.x, -Window_Resize_Thickness, 0);
  auto right_side  = in_range(p.x, width, width + Window_Resize_Thickness);
  auto top_side    = in_range(p.y, -Window_Resize_Thickness, 0);
  auto bottom_side = in_range(p.y, height, height + Window_Resize_Thickness);

  if (top_side)
  {
    if (left_side)  return left_top;
    if (right_side) return right_top;
    return top;
  }
  if (bottom_side)
  {
    if (left_side)  return left_bottom;
    if (right_side) return right_bottom;
    return bottom;
  }
  if (left_side)  return left;
  if (right_side) return right;
  return none;
}

}}
