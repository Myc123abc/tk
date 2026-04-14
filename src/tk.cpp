#include "tk.hpp"
#include "renderer/renderer/renderer.hpp"
#include "renderer/window/window_manager.hpp"
#include "ui/ui_context.hpp"
#include "util/thread_pool.hpp"

using namespace tk::renderer;
using namespace tk::ui;

namespace tk {

void init() noexcept
{
  g_thread_pool.init();
  g_renderer.init();
  g_wnd_mgr.init();
  g_ui_ctx.init();
}

void destroy() noexcept
{
  g_thread_pool.destroy();
  g_ui_ctx.destroy();
  g_renderer.destroy();
  g_wnd_mgr.destroy();
}

void update() noexcept
{
  g_ui_ctx.render();
  g_wnd_mgr.message_process();
  g_renderer.render();
}

}
