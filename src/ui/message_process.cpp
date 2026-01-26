#include "ui_context.hpp"

#include <algorithm>

namespace tk { namespace ui {

void UIContext::MessageHandler::operator()(Message_Window_Close const& msg) const noexcept
{
  if (auto it = std::ranges::find(ctx._windows, msg.handle, [](auto const& pair) { return pair.second.snap.handle; }); it != ctx._windows.end())
  {
    it->second.is_closed = true;
    return;
  }
  std::unreachable();
}

void UIContext::MessageHandler::operator()(Message_Cursor_On_Window const& msg) const noexcept
{
  ctx.cursor_on_window = msg.handle;
}

void UIContext::MessageHandler::operator()(Message_Update_Mouse_State const& msg) const noexcept
{
  if (std::ranges::any_of(ctx._mouse_state_queue, [=](auto message) { return message.state == msg.state; }))
    return;
  ctx._mouse_state_queue.emplace_back(msg);
}

void UIContext::process_mouse_state() noexcept
{
  if (_mouse_state_queue.empty()) return;
  _mouse_state = _mouse_state_queue.front().state;
  _mouse_state_queue.pop_front();
}

void UIContext::MessageHandler::operator()(Message_Update_Moving const& msg) const noexcept
{
  auto& wnd = ctx.get_window(msg.handle);
  wnd.snap.moving = msg.moving;
  wnd.snap.x      = msg.x;
  wnd.snap.y      = msg.y;
}

void UIContext::MessageHandler::operator()(Message_Update_Resizing const& msg) const noexcept
{
  auto& wnd = ctx.get_window(msg.handle);

  // when not resizing, first need to clear current window for rendering in fullscreen window
  if (wnd.snap.resizing == false)
  {
    wnd.snap.resizing = true;
    wnd.need_clear    = true;
  }

  wnd.snap.x      = msg.x;
  wnd.snap.y      = msg.y;
  wnd.snap.width  = msg.width;
  wnd.snap.height = msg.height;
}

void UIContext::MessageHandler::operator()(Message_Resize_End const& msg) const noexcept
{
  auto& wnd = ctx.get_window(msg.handle);
  wnd.snap.resizing                 = false;
  ctx._fullscreen_window.need_clear = true;
}

void UIContext::MessageHandler::operator()(Message_Window_Maximize const& msg) const noexcept
{
  auto& wnd = ctx.get_window(msg.handle);
  wnd.snap.maximized = true;
  wnd.snap.x         = msg.x;
  wnd.snap.y         = msg.y;
  wnd.snap.width     = msg.width;
  wnd.snap.height    = msg.height;
}

void UIContext::MessageHandler::operator()(Message_Window_Restore const& msg) const noexcept
{
  auto& wnd = ctx.get_window(msg.handle);
  wnd.snap.maximized = false;
  wnd.snap.x         = msg.x;
  wnd.snap.y         = msg.y;
  wnd.snap.width     = msg.width;
  wnd.snap.height    = msg.height;
}

void UIContext::MessageHandler::operator()(Message_Window_Moving_From_Maximize const& msg) const noexcept
{
  auto& wnd = ctx.get_window(msg.handle);
  wnd.snap.maximized            = false;
  wnd.snap.moving               = true;
  wnd.snap.x                    = msg.x;
  wnd.snap.y                    = msg.y;
  wnd.snap.width                = msg.width;
  wnd.snap.height               = msg.height;
  wnd.snap.move_from_maximize = true;
}

void UIContext::MessageHandler::operator()(Message_Window_Moving_From_Maximize_End const& msg) const noexcept
{
  auto& wnd = ctx.get_window(msg.handle);
  wnd.snap.moving               = false;
  wnd.snap.x                    = msg.x;
  wnd.snap.y                    = msg.y;
  wnd.snap.move_from_maximize = false;
}

void UIContext::MessageHandler::operator()(Message_Interruption const& msg) const noexcept
{
  ctx._interrupte = true;
}

void UIContext::MessageHandler::operator()(Message_Update_Fullscreen_Window const& msg) const noexcept
{
  ctx._fullscreen_window.snap.width  = msg.width;
  ctx._fullscreen_window.snap.height = msg.height;
}

}}
