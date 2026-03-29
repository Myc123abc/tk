#pragma once

#include "../util/singleton.hpp"

#include <wrl/client.h>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <dcomp.h>
#include <d3d11on12.h>

namespace tk::renderer {

template <typename To, typename From>
inline auto TryAs(Microsoft::WRL::ComPtr<From> const& ptr) noexcept
{
  auto res = Microsoft::WRL::ComPtr<To>{};
  ptr.As(&res);
  return res;
}

template <typename To, typename From>
inline auto TryAs(From ptr) noexcept
{
  auto res = Microsoft::WRL::ComPtr<To>{};
  ptr->QueryInterface(IID_PPV_ARGS(&res));
  return res;
}

Singleton(Core, g_core,
public:
  void init() noexcept;

  auto create_cmd_alloc(D3D12_COMMAND_LIST_TYPE type) const noexcept -> Microsoft::WRL::ComPtr<ID3D12CommandAllocator>;
  auto create_cmd(D3D12_COMMAND_LIST_TYPE type, ID3D12CommandAllocator* alloc) const noexcept -> Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList1>;
  
  auto factory()     const noexcept { return _factory.Get();     }
  auto device()      const noexcept { return _device.Get();      }
  auto comp_device() const noexcept { return _comp_device.Get(); }
  auto d2d_device()  const noexcept { return _d2d_device.Get();  }

private:
  void create_device_11on12() noexcept;

private:
  Microsoft::WRL::ComPtr<IDXGIFactory6>       _factory;
  Microsoft::WRL::ComPtr<ID3D12Device2>       _device;
  Microsoft::WRL::ComPtr<IDCompositionDevice> _comp_device;

  Microsoft::WRL::ComPtr<ID3D11On12Device>    _device_11on12;
  Microsoft::WRL::ComPtr<ID3D12CommandQueue>  _cmd_queue;
  Microsoft::WRL::ComPtr<ID2D1Device>         _d2d_device;
)

}
