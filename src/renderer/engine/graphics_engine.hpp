#pragma once

#include "slots.hpp"
#include "../resource/image.hpp"

namespace tk::renderer {

Singleton_Derive(GraphicsEngine, g_graphics_engine, Engine,
public:
  void init() noexcept
  {
    Engine::init(D3D12_COMMAND_LIST_TYPE_DIRECT);
    _slots.init(this);
  }

  void copy(Image& src, Image& dst) noexcept { _cpy_imgs.emplace_back(&src, &dst); }

  void acquire_slot() noexcept;
  auto submit_slot() noexcept -> uint64_t;

private:
  Slots<D3D12_COMMAND_LIST_TYPE_DIRECT>  _slots;
  std::vector<std::pair<Image*, Image*>> _cpy_imgs{};
)

}
