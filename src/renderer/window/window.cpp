#include "window.hpp"
#include "../renderer/renderer.hpp"
#include "window_manager.hpp"
#include "../../ui/ui_context.hpp"
#include "../config.hpp"
#include "util/error_handling.hpp"

using namespace tk::ui;

namespace
{

auto in_range(float v, float min, float max) noexcept
{
  return v >= min && v <= max;
}

}

namespace tk { namespace renderer {

void Window::init(int x, int y, uint32_t width, uint32_t height, float scale) noexcept
{
  _x      = x;
  _y      = y;
  _width  = width  * scale;
  _height = height * scale;
  _scale  = scale;
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
  _x      = x;
  _y      = y;
  _width  = width;
  _height = height;
  update_rect();

  _handle = CreateWindowExW(WS_EX_NOREDIRECTIONBITMAP | WS_EX_LAYERED | WS_EX_TRANSPARENT | WS_EX_TOOLWINDOW | WS_EX_TOPMOST,
    WindowManager::Auxiliary_Class, nullptr, WS_POPUP, x, y, width, height, 0, 0, GetModuleHandleW(nullptr), 0);
  err_if(!_handle, "failed to create window");

  // create window render resource
  g_renderer.send_message(Renderer::Message_Window_Create{ _handle, width, height });

  ShowWindow(_handle, SW_SHOW);
}

void Window::update_by_rect() noexcept
{
  _x      = _rect.left;
  _y      = _rect.top;
  _width  = _rect.right  - _rect.left;
  _height = _rect.bottom - _rect.top;
}

void Window::update_rect() noexcept
{
  _rect = { _x, _y, static_cast<LONG>(_x + _width), static_cast<LONG>(_y + _height) };
}

void Window::destroy() const noexcept
{
  ShowWindow(_handle, SW_HIDE);
  g_renderer.send_message(Renderer::Message_Window_Destroy{ _handle });
}

auto Window::cursor_pos() const noexcept -> glm::vec<2, int>
{
  auto pos = get_cursor_pos();
  return { pos.x - _x, pos.y - _y };
}

auto Window::cursor_valid_area() const noexcept -> RECT
{
  auto rect = _rect;
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

void Window::move_from_maximize() noexcept
{
  auto pos     = cursor_pos();
  auto ratio_x = static_cast<float>(pos.x) / _width;

  _move_from_maximize = true;
  _moving             = true;
  _maximized          = false;
  _width              = _backup_rect.right  - _backup_rect.left;
  _height             = _backup_rect.bottom - _backup_rect.top;
  _x                  = get_cursor_pos().x - _width * ratio_x;
  
  auto limit = Window_Y_Pos_Moving_From_Maximize * _scale;
  if (pos.y < limit)
    _y -= limit;

  update_rect();
  g_renderer.send_message(Renderer::Message_Window_Update{ _handle, real_width(), real_height() });
  g_ui_ctx.send_message(UIContext::Message_Window_Moving_From_Maximize{ _handle, _x, _y, _width, _height });
  SetWindowPos(_handle, 0, real_x(), real_y(), real_width(), real_height(), SWP_NOZORDER | SWP_NOACTIVATE);
}

void Window::monitor_change() noexcept
{
  auto rect = RECT{};

  // If the window is minimized (e.g., due to monitor removal), GetWindowRect
  // returns off-screen sentinel around (-32000, -32000). Use the normal position.
  if (IsIconic(_handle))
  {
    WINDOWPLACEMENT wp{ sizeof(wp) };
    if (GetWindowPlacement(_handle, &wp))
      rect = wp.rcNormalPosition;
    else
      GetWindowRect(_handle, &rect);
  }
  else
    GetWindowRect(_handle, &rect);

  rect.left   += shadow_thickness();
  rect.top    += shadow_thickness();
  rect.right  -= shadow_thickness();
  rect.bottom -= shadow_thickness();
  _rect      = rect;
  _maximized = false;
  update_by_rect();
  g_ui_ctx.send_message(UIContext::Message_Window_Restore{ _handle, _x, _y, _width, _height });

  // update fullscreen window
  g_wnd_mgr._fullscreen_window._rect = get_virtual_screen_rect();
  g_wnd_mgr._fullscreen_window.update_by_rect();
  g_renderer.send_message(Renderer::Message_Window_Update{
    g_wnd_mgr._fullscreen_window.handle(), g_wnd_mgr._fullscreen_window._width, g_wnd_mgr._fullscreen_window._height });
  g_ui_ctx.send_message(UIContext::Message_Update_Fullscreen_Window{ g_wnd_mgr._fullscreen_window._width, g_wnd_mgr._fullscreen_window._height });
  SetWindowPos(
    g_wnd_mgr._fullscreen_window.handle(), 0,
    g_wnd_mgr._fullscreen_window._x, g_wnd_mgr._fullscreen_window._y,
    g_wnd_mgr._fullscreen_window._width, g_wnd_mgr._fullscreen_window._height,
    SWP_NOZORDER | SWP_NOACTIVATE);
}

void Window::move_with_pos(int x, int y) noexcept
{
  _x = x;
  _y = y;
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
    g_ui_ctx.send_message(UIContext::Message_Window_Moving_From_Maximize_End{ _handle, _x, _y });
  }
  else
    g_ui_ctx.send_message(UIContext::Message_Update_Moving{ _handle, _moving, _x, _y });
}

void Window::adjust_offset(ResizeType type, glm::vec<2, int> const& point, int& dx, int& dy) const noexcept
{
  using enum ResizeType;
  switch (type)
  {
  case none:
    break;

  case left_top:
    if (_width  == min_width()  && point.x > _rect.left) dx = 0;
    if (_height == min_height() && point.y > _rect.top)  dy = 0;
    break;

  case right_top:
    if (_width  == min_width()  && point.x < _rect.right) dx = 0;
    if (_height == min_height() && point.y > _rect.top)   dy = 0;
    break;

  case left_bottom:
    if (_width  == min_width()  && point.x > _rect.left)   dx = 0;
    if (_height == min_height() && point.y < _rect.bottom) dy = 0;
    break;

  case right_bottom:
    if (_width  == min_width()  && point.x < _rect.right)  dx = 0;
    if (_height == min_height() && point.y < _rect.bottom) dy = 0;
    break;

  case left:
    if (_width == min_width() && point.x > _rect.left) dx = 0;
    break;

  case right:
    if (_width == min_width() && point.x < _rect.right) dx = 0;
    break;

  case top:
    if (_height == min_height() && point.y > _rect.top) dy = 0;
    break;

  case bottom:
    if (_height == min_height() && point.y < _rect.bottom) dy = 0;
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

  g_ui_ctx.send_message(UIContext::Message_Update_Resizing{ _handle, _x, _y, _width, _height });
}

void Window::resize_end() noexcept
{
  _resizing = false;
  g_ui_ctx.send_message(UIContext::Message_Resize_End{ _handle });
  g_renderer.send_message(Renderer::Message_Window_Update{ _handle, real_width(), real_height() });
  SetWindowPos(_handle, 0, real_x(), real_y(), real_width(), real_height(), SWP_NOZORDER | SWP_NOACTIVATE);
}

void Window::resize_by_scale(float scale) noexcept
{
  auto width  = _width  / _scale;
  auto height = _height / _scale;
  _scale  = scale;
  _width  = width  * scale;
  _height = height * scale;
  update_rect();

  // when the monitor scale is changed, if part of the window is outside the monitor,
  // the OS will move the window completely inside the monitor
  g_ui_ctx.send_message(UIContext::Message_Scale_Change{ _handle, scale, _x, _y, _width, _height });
  g_renderer.send_message(Renderer::Message_Window_Update{ _handle, real_width(), real_height() });
  SetWindowPos(_handle, 0, real_x(), real_y(), real_width(), real_height(), SWP_NOZORDER | SWP_NOACTIVATE);
}

void Window::left_offset(int dx) noexcept
{
  auto rc = get_virtual_screen_rect();
  _rect.left = std::clamp(
    _rect.left + dx,
    rc.left,
    static_cast<LONG>(std::min(_rect.right, rc.right) - min_width())
  );
  auto p = get_cursor_pos();
  if (dx < 0 && _rect.left < p.x && rc.right - _rect.left > min_width()) _rect.left = p.x;
  if (_rect.right > rc.right && rc.right - _rect.left < min_width()) _rect.left = rc.right - min_width();
}

void Window::top_offset(int dy) noexcept
{
  auto rc = get_virtual_screen_rect();
  _rect.top = std::clamp(
    _rect.top + dy,
    rc.top,
    static_cast<LONG>(std::min(_rect.bottom, rc.bottom) - min_height())
  );
  auto p = get_cursor_pos();
  if (dy < 0 && _rect.top < p.y && rc.bottom - _rect.top > min_height()) _rect.top = p.y;
  if (_rect.bottom > rc.bottom && rc.bottom - _rect.top < min_height()) _rect.top = rc.bottom - min_width();
}

void Window::right_offset(int dx) noexcept
{
  auto rc = get_virtual_screen_rect();
  _rect.right = std::clamp(
    _rect.right + dx,
    static_cast<LONG>(std::max(_rect.left, rc.left) + min_width()),
    rc.right
  );
  auto p = get_cursor_pos();
  if (dx > 0 && _rect.right > p.x && _rect.right - rc.left > min_width()) _rect.right = p.x;
  if (_rect.left < rc.left && _rect.right - rc.left < min_width()) _rect.right = rc.left + min_width();
  if (_rect.right == rc.right - 1) _rect.right = rc.right; // I don't know why in this case the dx is always 0
}

void Window::bottom_offset(int dy) noexcept
{
  auto rc = get_virtual_screen_rect();
  _rect.bottom = std::clamp(
    _rect.bottom + dy,
    static_cast<LONG>(std::max(_rect.top, rc.top) + min_height()),
    rc.bottom
  );
  auto p = get_cursor_pos();
  if (dy > 0 && _rect.bottom > p.y && _rect.bottom - rc.top > min_height()) _rect.bottom = p.y;
  if (_rect.top < rc.top && _rect.bottom - rc.top < min_height()) _rect.bottom = rc.top + min_height();
  if (_rect.bottom == rc.bottom - 1) _rect.bottom = rc.bottom; // I don't know why in this case the dy is always 0
}

void Window::maximize() noexcept
{
  _maximized   = true;
  _backup_rect =_rect;
  _rect        = get_window_monitor_work_rect(_handle);
  update_by_rect();
  g_renderer.send_message(Renderer::Message_Window_Update{ _handle, real_width(), real_height() });
  g_ui_ctx.send_message(UIContext::Message_Window_Maximize{ _handle, _x, _y, _width, _height });
  SetWindowPos(_handle, 0, real_x(), real_y(), real_width(), real_height(), SWP_NOZORDER | SWP_NOACTIVATE);
}

void Window::restore() noexcept
{
  _maximized = false;
  _rect      = _backup_rect;
  update_by_rect();
  g_renderer.send_message(Renderer::Message_Window_Update{ _handle, real_width(), real_height() });
  g_ui_ctx.send_message(UIContext::Message_Window_Restore{ _handle, _x, _y, _width, _height });
  SetWindowPos(_handle, 0, real_x(), real_y(), real_width(), real_height(), SWP_NOZORDER | SWP_NOACTIVATE);
}

auto Window::get_resize_type(glm::vec<2, int> const& p) const noexcept -> ResizeType
{
  using enum ResizeType;

  if (_maximized) return none;

  auto left_side   = in_range(p.x, -Window_Resize_Thickness, 0);
  auto right_side  = in_range(p.x, _width, _width + Window_Resize_Thickness);
  auto top_side    = in_range(p.y, -Window_Resize_Thickness, 0);
  auto bottom_side = in_range(p.y, _height, _height + Window_Resize_Thickness);

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
