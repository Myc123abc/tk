#include "graphics_engine.hpp"

namespace tk::renderer {

void GraphicsEngine::acquire_slot() noexcept
{
  _slots.acquire_slot();
}

auto GraphicsEngine::submit_slot() noexcept -> uint64
{
  return _slots.submit_slot();
}

}
