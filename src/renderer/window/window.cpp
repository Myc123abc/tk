#include "window.hpp"
#include "../renderer/renderer.hpp"
#include "window_manager.hpp"
#include "../../ui/ui_context.hpp"
#include "../config.hpp"
#include "util/error_handling.hpp"
#include "monitor.hpp"

using namespace tk::ui;

namespace
{

auto in_range(float v, float min, float max) noexcept
{
  return v >= min && v <= max;
}

}

namespace tk::renderer {

void Window::init(int x, int y, uint32_t width, uint32_t height, bool blur_backdrop) noexcept
{
  auto monitor = Monitor{ { x, y, static_cast<LONG>(x + width), static_cast<LONG>(y + height) } };

  _monitor = monitor.name();
  _scale   = monitor.scale();
  _x       = x;
  _y       = y;
  _width   = width  * _scale;
  _height  = height * _scale;
  update_rect();

  _handle = CreateWindowExW(WS_EX_NOREDIRECTIONBITMAP, WindowManager::Window_Class, nullptr, WS_POPUP | WS_MINIMIZEBOX,
    real_x(), real_y(), real_width(), real_height(), 0, 0, GetModuleHandleW(nullptr), 0);
  err_if(!_handle, "failed to create window");

  // move window to primary monitor if it's not on any display monitors
  auto work_area = monitor.work_rect();
  if (!point_on({ x, y }, work_area))
  {
    _x = work_area.left;
    _y = work_area.top;
    update_rect();
    SetWindowPos(_handle, 0, real_x(), real_y(), 0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
  }

  // create window render resource
  g_renderer.send_message(Renderer::Message_Window_Create{ _handle, real_width(), real_height() });

  if (blur_backdrop)
    init_blur_window();

  ShowWindow(_handle, SW_SHOW);
}

void Window::init_blur_window() noexcept
{
  _blur_window = CreateWindowExW(WS_EX_NOREDIRECTIONBITMAP | WS_EX_LAYERED | WS_EX_TRANSPARENT | WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE,
    WindowManager::Blur_Class, nullptr, WS_POPUP, _x, _y, _width, _height, 0, 0, GetModuleHandleW(nullptr), 0);
  err_if(!_handle, "failed to create window");
  
  // INFO: don't make blur window owned host as follow
  // SetWindowLongPtr(hwndBlur, GWLP_HWNDPARENT, (LONG_PTR)hwndHost);
  // this seem make dwm reset z-order lead g_wnd_mgr.blur_wnd_proc's WM_WINDOWPOSCHANGED's keep_blur_window_behind invalid.
  // lead blinking reappear!!!

  _blur_res = g_compositor.create_resource(_blur_window);

  // show blur window after first frame present complete
  g_renderer.send_message(Renderer::Message_Show_Blur_Window{ _handle, _blur_window });
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
  if (_blur_window)
    ShowWindow(_blur_window, SW_HIDE);
  ShowWindow(_handle, SW_HIDE);
  g_renderer.send_message(Renderer::Message_Window_Destroy{ _handle, _blur_window });
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

void Window::reset_pos_size() noexcept
{
  g_renderer.send_message(Renderer::Message_Window_Update{ _handle, real_width(), real_height() });
  g_ui_ctx.send_message(UIContext::Message_Window_Update{ _handle, _x, _y, _width, _height });
  SetWindowPos(_handle, 0, real_x(), real_y(), real_width(), real_height(), SWP_NOZORDER | SWP_NOACTIVATE);
  keep_blur_window_behind_resize();
}

void Window::update_by_rect(RECT rect, float scale) noexcept
{
  _rect  = rect;
  _scale = scale;
  update_by_rect();
  g_ui_ctx.send_message(UIContext::Message_Scale_Change{ _handle, scale, _x, _y, _width, _height });
  g_renderer.send_message(Renderer::Message_Window_Update{ _handle, real_width(), real_height() });
  SetWindowPos(_handle, 0, real_x(), real_y(), real_width(), real_height(), SWP_NOZORDER | SWP_NOACTIVATE);
  keep_blur_window_behind_resize();
}

void Window::update_by_real_rect(RECT rect, float scale) noexcept
{
  _scale  = scale;
  _x      = rect.left + shadow_thickness();
  _y      = rect.top  + shadow_thickness();
  _width  = (rect.right  - rect.left) - shadow_thickness() * 2;
  _height = (rect.bottom - rect.top)  - shadow_thickness() * 2;
  update_rect();
  g_ui_ctx.send_message(UIContext::Message_Scale_Change{ _handle, scale, _x, _y, _width, _height });
  g_renderer.send_message(Renderer::Message_Window_Update{ _handle, real_width(), real_height() });
  SetWindowPos(_handle, 0, real_x(), real_y(), real_width(), real_height(), SWP_NOZORDER | SWP_NOACTIVATE);
  keep_blur_window_behind_resize();
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

void Window::resize_by_scale(float scale, float ratio, glm::vec2 cursor_pos, glm::vec2 left_button_down_window_cusor_pos) noexcept
{
  _x       = cursor_pos.x - left_button_down_window_cusor_pos.x;
  _y       = cursor_pos.y - left_button_down_window_cusor_pos.y;
  _width  *= ratio;
  _height *= ratio;
  _scale   = scale;

  update_rect();

  g_ui_ctx.send_message(UIContext::Message_Scale_Change{ _handle, scale, _x, _y, _width, _height });
  g_renderer.send_message(Renderer::Message_Window_Update{ _handle, real_width(), real_height() });
  SetWindowPos(_handle, 0, real_x(), real_y(), real_width(), real_height(), SWP_NOZORDER | SWP_NOACTIVATE);
  keep_blur_window_behind_resize();
}

void Window::left_offset(int dx) noexcept
{
  auto rc = get_virtual_screen_rect();

  LONG new_left = _rect.left + dx;

  // enforce min width
  LONG max_left = _rect.right - min_width();

  // prevent moving past desktop
  LONG min_left = rc.left;

  new_left = std::clamp(new_left, min_left, max_left);

  auto p = get_cursor_pos();
  if (dx < 0 && new_left < p.x)
    new_left = std::min((LONG)p.x, max_left);

  _rect.left = new_left;
}

void Window::top_offset(int dy) noexcept
{
  auto rc = get_virtual_screen_rect();

  LONG new_top = _rect.top + dy;

  LONG max_top = _rect.bottom - min_height();
  LONG min_top = rc.top;

  new_top = std::clamp(new_top, min_top, max_top);

  auto p = get_cursor_pos();
  if (dy < 0 && new_top < p.y)
    new_top = std::min((LONG)p.y, max_top);

  _rect.top = new_top;
}

void Window::right_offset(int dx) noexcept
{
  auto rc = get_virtual_screen_rect();

  LONG new_right = _rect.right + dx;

  LONG min_right = _rect.left + min_width();
  LONG max_right = rc.right;

  new_right = std::clamp(new_right, min_right, max_right);

  auto p = get_cursor_pos();
  if (dx > 0 && new_right > p.x)
    new_right = std::max((LONG)p.x, min_right);

  _rect.right = new_right;
}

void Window::bottom_offset(int dy) noexcept
{
  auto rc = get_virtual_screen_rect();

  LONG new_bottom = _rect.bottom + dy;

  LONG min_bottom = _rect.top + min_height();
  LONG max_bottom = rc.bottom;

  new_bottom = std::clamp(new_bottom, min_bottom, max_bottom);

  auto p = get_cursor_pos();
  if (dy > 0 && new_bottom > p.y)
    new_bottom = std::max((LONG)p.y, min_bottom);

  _rect.bottom = new_bottom;
}

void Window::maximize() noexcept
{
  _maximized   = true;
  _backup_rect = _rect;
  _rect        = Monitor{ _handle }.work_rect();
  update_by_rect();
  g_renderer.send_message(Renderer::Message_Window_Update{ _handle, real_width(), real_height() });
  g_ui_ctx.send_message(UIContext::Message_Window_Maximize{ _handle, _x, _y, _width, _height });
  SetWindowPos(_handle, 0, real_x(), real_y(), real_width(), real_height(), SWP_NOZORDER | SWP_NOACTIVATE);
  keep_blur_window_behind_resize();
}

void Window::minimize() noexcept
{
  if (_blur_window)
    ShowWindow(_blur_window, SW_MINIMIZE);
  ShowWindow(_handle, SW_MINIMIZE);
}

void Window::cancel_maximize(RECT rect, float scale) noexcept
{
  _maximized = false;
  g_ui_ctx.send_message(UIContext::Message_Window_Restore{ _handle, _x, _y, _width, _height });
  update_by_real_rect(rect, scale);
  keep_blur_window_behind_resize();
}

void Window::cancel_fullscreen(RECT rect, float scale) noexcept
{
  _fullscreen = false;
  g_ui_ctx.send_message(UIContext::Message_Window_Restore_Fullscreen{ _handle, _x, _y, _width, _height });
  update_by_real_rect(rect, scale);
  keep_blur_window_behind_resize();
}

void Window::cancel_fullscreen_maximize(RECT rect, float scale) noexcept
{
  auto monitor = Monitor{ rect };
  if (_maximized)  rect = monitor.work_rect();
  if (_fullscreen) rect = monitor.rect();
  _maximized  = false;
  _fullscreen = false; 
  g_ui_ctx.send_message(UIContext::Message_Window_Cancel_Fullscreen_Maximize{ _handle, _x, _y, _width, _height });
  update_by_real_rect(rect, scale);
  keep_blur_window_behind_resize();
}

void Window::restore() noexcept
{
  _maximized = false;
  _rect      = _backup_rect;
  update_by_rect();
  g_ui_ctx.send_message(UIContext::Message_Window_Restore{ _handle, _x, _y, _width, _height });
  g_renderer.send_message(Renderer::Message_Window_Update{ _handle, real_width(), real_height() });
  SetWindowPos(_handle, 0, real_x(), real_y(), real_width(), real_height(), SWP_NOZORDER | SWP_NOACTIVATE);
  keep_blur_window_behind_resize();
}

void Window::fullscreen() noexcept
{
  _maximized   = false;
  _fullscreen  = true;
  _backup_rect = _rect;
  _rect        = Monitor{ _handle }.rect();
  update_by_rect();
  g_renderer.send_message(Renderer::Message_Window_Update{ _handle, real_width(), real_height() });
  g_ui_ctx.send_message(UIContext::Message_Window_Fullscreen{ _handle, _x, _y, _width, _height });
  SetWindowPos(_handle, 0, real_x(), real_y(), real_width(), real_height(), SWP_NOZORDER | SWP_NOACTIVATE);
  keep_blur_window_behind_resize();
}

void Window::restore_fullscreen() noexcept
{
  _fullscreen = false;
  _rect       = _backup_rect;
  update_by_rect();
  g_ui_ctx.send_message(UIContext::Message_Window_Restore_Fullscreen{ _handle, _x, _y, _width, _height });
  g_renderer.send_message(Renderer::Message_Window_Update{ _handle, real_width(), real_height() });
  SetWindowPos(_handle, 0, real_x(), real_y(), real_width(), real_height(), SWP_NOZORDER | SWP_NOACTIVATE);
  keep_blur_window_behind_resize();
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

void Window::show_blur_window() const noexcept
{
  ShowWindow(_blur_window, SW_SHOW);
  keep_blur_window_behind();
}

void Window::keep_blur_window_behind() const noexcept
{
  if (_blur_window)
    SetWindowPos(_blur_window, _handle, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE | SWP_NOOWNERZORDER);
}

void Window::keep_blur_window_behind_move() const noexcept
{
  if (_blur_window)
    SetWindowPos(_blur_window, _handle, _x, _y, 0, 0, SWP_NOSIZE | SWP_NOACTIVATE | SWP_NOOWNERZORDER);
}

void Window::keep_blur_window_behind_resize() const noexcept
{
  if (_blur_window)
    SetWindowPos(_blur_window, _handle, _x, _y, _width, _height, SWP_NOACTIVATE | SWP_NOOWNERZORDER);
}

}
