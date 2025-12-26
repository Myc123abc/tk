#include "ui_context.hpp"

namespace tk { namespace ui {

void UIContext::MessageHandler::operator()(Message_Window_Close const& msg) const noexcept
{
  if (auto it = std::ranges::find(g_ui_ctx._windows, msg.handle, [](auto const& pair) { return pair.second.handle; }); it != g_ui_ctx._windows.end())
  {
    it->second.is_closed = true;
    return;
  }
  std::unreachable();
}

}}
