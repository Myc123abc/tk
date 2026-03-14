#pragma once

#include "engine.hpp"
#include "../../util/singleton.hpp"

#include <vector>

namespace tk { namespace renderer {

Singleton_Derive(ComputeEngine, g_comp_engine, Engine,
public:
  void init() noexcept { Engine::init(D3D12_COMMAND_LIST_TYPE_COMPUTE); }

  void acquire_slot() noexcept;

  [[nodiscard]]
  auto submit_slot() noexcept -> uint64_t;

private:
  struct Slot
  {
    Microsoft::WRL::ComPtr<ID3D12CommandAllocator> cmd_alloc;
    uint64_t                                       fence_value{};  

    auto is_idle() const noexcept -> bool;

    Slot() noexcept;
  };

  std::vector<Slot> _slots;
  Slot*             _slot{};
)

}}
