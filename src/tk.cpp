#include "tk.hpp"
#include "renderer/renderer.hpp"
#include "renderer/window/window_manager.hpp"

using namespace tk::renderer;

namespace tk {

void init() noexcept
{
  g_wnd_mgr.init();
  g_renderer.init();
}

void destroy() noexcept
{
  g_renderer.destroy();
  g_wnd_mgr.destroy();
}

auto window_count() noexcept -> uint32_t
{
  return g_wnd_mgr.window_count();
}

void test()
{
  g_wnd_mgr.create_window(20, 20, 200, 200);
  g_wnd_mgr.create_window(70, 70, 200, 200);
}

}
