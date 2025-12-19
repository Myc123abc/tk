#include "device.hpp"
#include "../util/error_handling.hpp"
#include "resource/descriptor_heap_manager.hpp"

#include <dxgi1_6.h>

using namespace Microsoft::WRL;

namespace tk { namespace renderer {

void Device::init() noexcept
{
  // init debug controller
  #ifndef NDEBUG
  auto debug_controller = ComPtr<ID3D12Debug>{};
  err_if(D3D12GetDebugInterface(IID_PPV_ARGS(&debug_controller)),
          "failed to create d3d12 debug controller");
  debug_controller->EnableDebugLayer();
  #endif

  // create factory
  auto factory = ComPtr<IDXGIFactory6>{};
  #ifndef NDEBUG
  err_if(CreateDXGIFactory2(DXGI_CREATE_FACTORY_DEBUG, IID_PPV_ARGS(&factory)),
          "failed to create dxgi factory");
  #else
  err_if(CreateDXGIFactory2(0, IID_PPV_ARGS(&factory)),
          "failed to create dxgi factory");
  #endif

  // create device
  auto adapter = ComPtr<IDXGIAdapter4>{};
  err_if(factory->EnumAdapterByGpuPreference(0, DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE, IID_PPV_ARGS(&adapter)),
          "failed to enum dxgi adapter");
  err_if(D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_12_1, IID_PPV_ARGS(&_device)),
          "failed to create d3d12 device");

  // get render target view descriptor size
  RTV_Size         = _device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
  CBV_SRV_UAV_Size = _device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
  DSV_Size         = _device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
}

}}
