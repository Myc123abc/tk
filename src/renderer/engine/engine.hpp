#pragma once

#include <d3d12.h>
#include <wrl/client.h>

#include <stdint.h>
#include <optional>

#include "util/base.hpp"
#include "../resource/command.hpp"

namespace tk::renderer {

class Engine
{
public:
  Engine()                         = default;
  ~Engine()                        = default;
  Engine(Engine const&)            = delete;
  Engine(Engine&&)                 = delete;
  Engine& operator=(Engine const&) = delete;
  Engine& operator=(Engine&&)      = delete;

  auto signal() noexcept -> uint64;
  auto submit() noexcept -> uint64;

  auto fence_completed_value() const noexcept { return _fence->GetCompletedValue(); }

  void wait(Engine const& engine, std::optional<uint64> fence_value = {}) const noexcept;

  auto queue() const noexcept { return &_queue; }

  auto set_event_on_completion() const noexcept -> HANDLE;

  void destroy() noexcept;

  auto reset_cmd(ID3D12CommandAllocator* alloc) const noexcept { return g_cmd_pool.reinit(_cmd, alloc); }
  auto reset_cmd() const noexcept { return reset_cmd(_cmd_alloc.Get()); }
  auto cmd() const noexcept { return g_cmd_pool.get(_cmd); }

protected:
  void init(D3D12_COMMAND_LIST_TYPE type) noexcept;

private:
  CmdQueue                                       _queue;
  Microsoft::WRL::ComPtr<ID3D12CommandAllocator> _cmd_alloc;
  CmdHandle                                      _cmd;
  Microsoft::WRL::ComPtr<ID3D12Fence>            _fence;
  uint64                                         _fence_value{};
  HANDLE                                         _fence_event{};
};

}
