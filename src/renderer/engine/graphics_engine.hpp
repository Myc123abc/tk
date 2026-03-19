#pragma once

#include "engine.hpp"
#include "../../util/singleton.hpp"

namespace tk::renderer {

Singleton_Derive(GraphicsEngine, g_graphics_engine, Engine,
public:
  void init() noexcept { Engine::init(D3D12_COMMAND_LIST_TYPE_DIRECT); }
)

}
