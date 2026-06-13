#pragma once

#include "slots.hpp"
#include "../resource/image_manager.hpp"

namespace tk::renderer {

Singleton_Derive(GraphicsEngine, g_graphics_engine, Engine,
public:
  void init() noexcept
  {
    Engine::init(D3D12_COMMAND_LIST_TYPE_DIRECT);
    _slots.init(this);
  }

  void copy(ImageHandle src, ImageHandle dst, int right, int bottom, int left = {}, int top = {}, uint2 pos = {}) noexcept
  { _cpy_imgs.emplace_back(src, Rect{ left, top, right, bottom }, dst, pos); }

  void acquire_slot() noexcept;
  auto submit_slot() noexcept -> uint64;

private:
  Slots<D3D12_COMMAND_LIST_TYPE_DIRECT>  _slots;

  struct CopyImage
  {
    ImageHandle src;
    Rect        rect;
    ImageHandle dst;
    uint2       pos;
  };
  std::vector<CopyImage> _cpy_imgs;
)

}
