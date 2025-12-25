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

}
