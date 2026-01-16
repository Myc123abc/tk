#pragma once

#include "engine.hpp"

namespace tk { namespace renderer {

class GraphicsEngine final : public Engine
{
public:
  static auto instance() noexcept -> GraphicsEngine&
  {
    static GraphicsEngine instance;
    return instance;
  }

  void init() noexcept { Engine::init(D3D12_COMMAND_LIST_TYPE_DIRECT); }
};

inline static auto& g_graphics_engine{ GraphicsEngine::instance() };

}}
