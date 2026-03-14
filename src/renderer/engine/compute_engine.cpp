#include "compute_engine.hpp"
#include "../core.hpp"
#include "../resource/descriptor_heap_manager.hpp"

#include <assert.h>

namespace tk { namespace renderer {

auto ComputeEngine::Slot::is_idle() const noexcept -> bool
{
  return g_comp_engine.fence_completed_value() >= fence_value;
}

ComputeEngine::Slot::Slot() noexcept
{
  cmd_alloc = g_core.create_cmd_alloc(D3D12_COMMAND_LIST_TYPE_COMPUTE);
}

void ComputeEngine::acquire_slot() noexcept
{
  if (auto it = std::ranges::find_if(_slots, [this](auto slot) { return slot.is_idle(); });
      it != _slots.end())
  {
    _slot = &*it;
  }
  else
  {
    _slots.emplace_back(Slot{});
    _slot = &_slots.back();
  }
  reset_cmd(_slot->cmd_alloc.Get());

  // bind heaps
  g_desc_heap_mgr.bind_heaps(cmd());
}

auto ComputeEngine::submit_slot() noexcept -> uint64_t
{
  assert(_slot && _slot->is_idle());
  _slot->fence_value = submit(); 
  return _slot->fence_value;
}


}}
