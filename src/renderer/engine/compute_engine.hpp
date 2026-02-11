#pragma once

#include "engine.hpp"

namespace tk { namespace renderer {

class ComputeEngine final : public Engine
{
public:
  static auto instance() noexcept -> ComputeEngine&
  {
    static ComputeEngine instance;
    return instance;
  }

  void init() noexcept { Engine::init(D3D12_COMMAND_LIST_TYPE_COMPUTE); }
};

inline static auto& g_compute_engine{ ComputeEngine::instance() };

}}
