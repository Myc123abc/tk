export module tk;

export import :log;

import :window;

namespace tk {

using namespace window;

export void init() noexcept
{
  g_wnd_mgr.init();
}

export void destroy() noexcept
{
  g_wnd_mgr.destroy();
}

export auto window_count() noexcept
{
  return g_wnd_mgr.window_count();
}

export void test()
{
  g_wnd_mgr.create_window(20, 20, 200, 200);
}

}
