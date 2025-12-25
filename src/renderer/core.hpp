#pragma once

#include <wrl/client.h>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <dcomp.h>

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

  auto create_cmd_alloc(D3D12_COMMAND_LIST_TYPE type) const noexcept -> Microsoft::WRL::ComPtr<ID3D12CommandAllocator>;
  auto create_cmd(D3D12_COMMAND_LIST_TYPE type, ID3D12CommandAllocator* alloc) const noexcept -> Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList1>;
  
  auto factory()     const noexcept { return _factory.Get();      }
  auto device()      const noexcept { return _device.Get();       }
  auto comp_device() const noexcept { return _comp_device.Get();  }

private:
  Microsoft::WRL::ComPtr<IDXGIFactory6>       _factory;
  Microsoft::WRL::ComPtr<ID3D12Device2>       _device;
  Microsoft::WRL::ComPtr<IDCompositionDevice> _comp_device;
};

inline static auto& g_core{ Core::instance() };

}}
