#pragma once

#include "engine.hpp"

namespace tk { namespace renderer {

class CopyEngine : public Engine
{
private:
  CopyEngine()                             = default;
  ~CopyEngine()                            = default;

public:
  static auto instance() noexcept -> CopyEngine&
  {
    static CopyEngine instance;
    return instance;
  }

  void init() noexcept { Engine::init(D3D12_COMMAND_LIST_TYPE_COPY); }
};

inline static auto& g_copy_engine{ CopyEngine::instance() };

}}
