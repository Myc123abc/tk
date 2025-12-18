#pragma once

#include <wrl/client.h>
#include <d3d12.h>
#include <dxgi1_6.h>

#include <stdint.h>

namespace tk { namespace renderer {

class Core
{
private:
  Core()                       = default;
  ~Core()                      = default;
public:
  Core(Core const&)            = delete;
  Core(Core&&)                 = delete;
  Core& operator=(Core const&) = delete;
  Core& operator=(Core&&)      = delete;

  static auto instance() noexcept -> Core&
  {
    static Core instance;
    return instance;
  }

  void init() noexcept;
  void destroy() const noexcept;

  auto signal(ID3D12CommandQueue* queue) noexcept -> uint64_t;

  auto fence_completed_value() const noexcept { return _fence->GetCompletedValue(); }
  
  auto device() const noexcept { return _device.Get(); }

private:
  Microsoft::WRL::ComPtr<IDXGIFactory6> _factory;
  Microsoft::WRL::ComPtr<ID3D12Device2> _device;
  Microsoft::WRL::ComPtr<ID3D12Fence>   _fence;
  HANDLE                                _fence_event{};
  uint64_t                              _fence_value{};
};

inline static auto& g_core{ Core::instance() };

}}
