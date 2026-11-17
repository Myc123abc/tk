module;

#include <algorithm>
#include <thread>

module tk.ui;

import tk.async;
import tk.file_system;
import tk.renderer;
import tk.ui.context;
import tk.ui.text.glyph_cacher;

using namespace tk::renderer;
using namespace tk::ui;

namespace tk {

void init() noexcept
{
  g_iocp.init();
  // reserve one thread for main, and one thread for audio
  g_thread_pool.init(std::max(1u, std::thread::hardware_concurrency() - 2));
  g_glyph_cacher.init();
  g_renderer.init();
  g_wnd_mgr.init();
  ui::g_ctx.init();
}

void destroy() noexcept
{
  update();
  g_glyph_cacher.destroy();
  g_renderer.wait_idle();
  g_thread_pool.destroy();
  ui::g_ctx.destroy();
  g_renderer.destroy();
  g_wnd_mgr.destroy();
  g_file_sys.destroy();
  g_iocp.destroy();
}

void update() noexcept
{
  g_img_mgr.update();
  ui::g_ctx.render();
  g_wnd_mgr.message_process();
  g_renderer.render();
  ui::g_ctx.postprocess();
}

}
