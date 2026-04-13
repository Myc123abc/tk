#include "tk.hpp"
#include "renderer/renderer/renderer.hpp"
#include "renderer/window/window_manager.hpp"
#include "ui/ui_context.hpp"

using namespace tk::renderer;
using namespace tk::ui;

namespace tk {

void init() noexcept
{
  g_wnd_mgr.init();
  g_renderer.init();
  g_ui_ctx.init();
}

void destroy() noexcept
{
  g_ui_ctx.destroy();
  g_renderer.destroy();
  g_wnd_mgr.destroy();
}

}
