module tk.ui;

import :common;
import tk.ui.context;

namespace tk::ui {

auto delta_time() noexcept -> double
{
  return g_ctx.delta_time();
}

}
