module;

#include <d3d12.h>

export module tk.renderer:engine.graphics_engine;

import :engine;
import :engine.slot;
import tk.singleton;

export namespace tk::renderer {

class GraphicsEngine : public Singleton<GraphicsEngine>, public Engine
{
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
};

auto& g_graphics_engine = GraphicsEngine::instance();

}
