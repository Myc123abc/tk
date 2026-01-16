#pragma once

#include <d3d12.h>
#include <wrl/client.h>

#include <stdint.h>
#include <optional>

namespace tk { namespace renderer {

class Engine
{
public:
  Engine()                         = default;
  ~Engine()                        = default;
  Engine(Engine const&)            = delete;
  Engine(Engine&&)                 = delete;
  Engine& operator=(Engine const&) = delete;
  Engine& operator=(Engine&&)      = delete;

  auto signal() noexcept -> uint64_t;
  [[nodiscard]]
  auto submit() noexcept -> uint64_t;

  auto fence_completed_value() const noexcept { return _fence->GetCompletedValue(); }

  void wait(Engine const& engine, std::optional<uint64_t> fence_value = {}) const noexcept;

  auto queue() const noexcept { return _queue.Get(); }

  auto set_event_on_completion() const noexcept -> HANDLE;

  void destroy() noexcept;

  auto reset_cmd(ID3D12CommandAllocator* alloc) const noexcept -> ID3D12GraphicsCommandList1*;
  auto cmd() const noexcept { return _cmd.Get(); }

protected:
  void init(D3D12_COMMAND_LIST_TYPE type) noexcept;

private:
  Microsoft::WRL::ComPtr<ID3D12CommandQueue>         _queue;
  Microsoft::WRL::ComPtr<ID3D12CommandAllocator>     _cmd_alloc;
  Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList1> _cmd;
  Microsoft::WRL::ComPtr<ID3D12Fence>                _fence;
  uint64_t                                           _fence_value{};
  HANDLE                                             _fence_event{};
};

}}
