#pragma once

#include "../core.hpp"
#include "submission_tracker.hpp"

#include <vector>
#include <assert.h>

namespace tk::renderer {

template <D3D12_COMMAND_LIST_TYPE CmdType, typename DataType = int>
class Slots
{
public:
  void init(Engine* engine) noexcept { _engine = engine; }

  auto acquire_slot() noexcept
  {
    auto need_init = false;
    if (auto it = std::ranges::find_if(_slots, [this](auto const& slot) { return is_idle(slot); });
        it != _slots.end())
    {
      _slot = &*it;
    }
    else
    {
      _slots.emplace_back(Slot{});
      _slot = &_slots.back();
      need_init = true;
    }
    _engine->reset_cmd(_slot->cmd_alloc.Get());
    return need_init;
  }

  auto submit_slot() noexcept -> uint64
  {
    assert(_slot && is_idle(_slot));
    _slot->fence_value = g_sub_tracker.submit(*_engine); 
    return _slot->fence_value;
  }

  auto slot() noexcept { return _slot; }

  auto begin() noexcept { return _slots.begin(); }
  auto end() noexcept { return _slots.end(); }

  auto begin() const noexcept { return _slots.begin(); }
  auto end() const noexcept { return _slots.end(); }

private:
  struct Slot
  {
    Microsoft::WRL::ComPtr<ID3D12CommandAllocator> cmd_alloc;
    uint64                                         fence_value{};
    DataType                                       data;

    Slot() noexcept
    {
      cmd_alloc = g_core.create_cmd_alloc(CmdType);
    }
  };

public:
  auto is_idle(Slot const& slot) const noexcept { return _engine->fence_completed_value() >= slot.fence_value; }
  auto is_idle(Slot* slot) const noexcept { return _engine->fence_completed_value() >= slot->fence_value; }

private:
  std::vector<Slot> _slots;
  Slot*             _slot{};
  Engine*           _engine{};
};

}
