module tk.ui;

import :common;
import tk.string_id;
import tk.ui.context;

namespace tk::ui {

auto delta_time() noexcept -> double
{
  return g_ctx.delta_time();
}

void push_id(StringID name) noexcept
{
  g_ctx.push_id(name);
}

void pop_id() noexcept
{
  g_ctx.pop_id();
}

}
