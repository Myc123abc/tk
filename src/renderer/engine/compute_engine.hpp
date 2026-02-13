#pragma once

#include "engine.hpp"

#include <vector>

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
};

inline static auto& g_compute_engine{ ComputeEngine::instance() };

}}
