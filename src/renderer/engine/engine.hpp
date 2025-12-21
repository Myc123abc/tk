#pragma once

#include <d3d12.h>
#include <wrl/client.h>

#include <stdint.h>

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

  auto fence_completed_value() const noexcept { return _fence->GetCompletedValue(); }

  auto queue() const noexcept { return _queue.Get(); }

protected:
  void init(D3D12_COMMAND_LIST_TYPE type) noexcept;

private:
  Microsoft::WRL::ComPtr<ID3D12CommandQueue>         _queue;
  Microsoft::WRL::ComPtr<ID3D12CommandAllocator>     _cmd_alloc;
  Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList1> _cmd;
  Microsoft::WRL::ComPtr<ID3D12Fence>                _fence;
  uint64_t                                           _fence_value{};
};

}}
