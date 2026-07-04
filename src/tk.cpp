#include "tk.hpp"
#include "renderer/renderer.hpp"
#include "renderer/window/window_manager.hpp"
#include "ui/ui_context.hpp"
#include "util/thread_pool.hpp"
#include "util/file_manager.hpp"

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
  update();
  g_renderer.wait_idle();
  g_thread_pool.destroy();
  g_ui_ctx.destroy();
  g_renderer.destroy();
  g_wnd_mgr.destroy();
  g_file_mgr.destroy();
}

void update() noexcept
{
  g_img_mgr.update();
  g_ui_ctx.render();
  g_wnd_mgr.message_process();
  g_renderer.render();
  g_ui_ctx.postprocess();
}

}
