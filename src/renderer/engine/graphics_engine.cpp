#include "graphics_engine.hpp"

namespace tk::renderer {

void GraphicsEngine::acquire_slot() noexcept
{
  _slots.acquire_slot();
}

auto GraphicsEngine::submit_slot() noexcept -> uint64
{
  if (_cpy_imgs.empty()) return 0;

  auto cmd = Engine::cmd();

  for (auto [src, rect, dst, pos] : _cpy_imgs)
    renderer::copy(cmd, g_img_mgr[src], rect.x, rect.y, rect.z, rect.w, g_img_mgr[dst], pos.x, pos.y);
  _cpy_imgs.clear();

  return _slots.submit_slot();
}

}
