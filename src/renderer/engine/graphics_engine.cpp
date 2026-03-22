#include "graphics_engine.hpp"

namespace tk::renderer {

void GraphicsEngine::acquire_slot() noexcept
{
  _slots.acquire_slot();
}

auto GraphicsEngine::submit_slot() noexcept -> uint64_t
{
  for (auto [src, dst] : _cpy_imgs)
    renderer::copy(cmd(), *src, *dst);
  return _slots.submit_slot();
}

}
