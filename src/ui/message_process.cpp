#include "ui_context.hpp"

namespace tk { namespace ui {

void UIContext::MessageHandler::operator()(Message_Window_Close msg) const noexcept
{
  if (auto it = std::ranges::find(ctx._windows, msg.handle, [](auto const& pair) { return pair.second.snap.handle; }); it != ctx._windows.end())
  {
    it->second.is_closed = true;
    return;
  }
  std::unreachable();
}

void UIContext::MessageHandler::operator()(Message_Cursor_On_Window msg) const noexcept
{
  ctx.cursor_on_window = msg.handle;
}

void UIContext::MessageHandler::operator()(Message_Update_Mouse_State msg) const noexcept
{
  if (std::ranges::any_of(ctx._mouse_state_queue, [=](auto message) { return message.state == msg.state; }))
    return;
  ctx._mouse_state_queue.emplace_back(msg);
}

void UIContext::process_mouse_state() noexcept
{
  if (_mouse_state_queue.empty()) return;
  auto msg = _mouse_state_queue.front();
  _mouse_state = msg.state;
  _mouse_state_queue.pop_back();
}

void UIContext::MessageHandler::operator()(Message_Update_Moving msg) const noexcept
{
  auto& wnd = ctx.get_window(msg.handle);
  wnd.snap.moving = msg.moving;
  wnd.snap.x      = msg.x;
  wnd.snap.y      = msg.y;
}

}}
