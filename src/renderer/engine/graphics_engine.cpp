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
    cmd->copy(src, rect, dst, pos);
  _cpy_imgs.clear();

  return _slots.submit_slot();
}

}
