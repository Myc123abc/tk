#include "core.hpp"
#include "../util/error_handling.hpp"
#include "resource/descriptor_heap_manager.hpp"

using namespace Microsoft::WRL;

namespace tk { namespace renderer {

void Core::init() noexcept
{
  // init debug controller
  #ifndef NDEBUG
  ComPtr<ID3D12Debug> debug_controller;
  err_if(D3D12GetDebugInterface(IID_PPV_ARGS(&debug_controller)),
          "failed to create d3d12 debug controller");
  debug_controller->EnableDebugLayer();
  #endif

  // create factory
  #ifndef NDEBUG
  err_if(CreateDXGIFactory2(DXGI_CREATE_FACTORY_DEBUG, IID_PPV_ARGS(&_factory)),
          "failed to create dxgi factory");
  #else
  err_if(CreateDXGIFactory2(0, IID_PPV_ARGS(&_factory)),
          "failed to create dxgi factory");
  #endif

  // create device
  ComPtr<IDXGIAdapter4> adapter;
  err_if(_factory->EnumAdapterByGpuPreference(0, DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE, IID_PPV_ARGS(&adapter)),
          "failed to enum dxgi adapter");
  err_if(D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_12_1, IID_PPV_ARGS(&_device)),
          "failed to create d3d12 device");

  // get render target view descriptor size
  RTV_Size         = _device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
  CBV_SRV_UAV_Size = _device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
  DSV_Size         = _device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);

  // create fence resources
  err_if(_device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&_fence)),
          "failed to create fence");
  _fence_event = CreateEvent(nullptr, false, false, nullptr);
  err_if(!_fence_event, "failed to create fence event");
}

void Core::destroy() const noexcept
{
  CloseHandle(_fence_event);
}

auto Core::signal(ID3D12CommandQueue* queue) noexcept -> uint64_t
{
  err_if(queue->Signal(_fence.Get(), ++_fence_value), "failed to signal fence");
  return _fence_value;
}

}}
