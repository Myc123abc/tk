#include "ui_context.hpp"

namespace tk { namespace ui {

void UIContext::MessageHandler::operator()(Message_Window_Close msg) const noexcept
{
  if (auto it = std::ranges::find(ctx._windows, msg.handle, [](auto const& pair) { return pair.second.handle; }); it != ctx._windows.end())
  {
    it->second.is_closed = true;
    return;
  }
  std::unreachable();
}

}}
