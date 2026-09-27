#pragma once

#include "slots.hpp"

namespace tk::renderer {

Singleton_Derive(GraphicsEngine, g_graphics_engine, Engine,
public:
  void init() noexcept
  {
    Engine::init(D3D12_COMMAND_LIST_TYPE_DIRECT);
    _slots.init(this);
  }

  void acquire_slot() noexcept;
  auto submit_slot() noexcept -> uint64;

private:
  Slots<D3D12_COMMAND_LIST_TYPE_DIRECT> _slots;
)

}
