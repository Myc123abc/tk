#include "core.hpp"
#include "util/error_handling.hpp"
#include "resource/descriptor_heap_manager.hpp"

using namespace Microsoft::WRL;

namespace tk::renderer {

void Core::init() noexcept
{
  // init debug controller
#ifndef NDEBUG
  auto debug_controller = ComPtr<ID3D12Debug>{};
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
  auto adapter = ComPtr<IDXGIAdapter4>{};
  err_if(_factory->EnumAdapterByGpuPreference(0, DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE, IID_PPV_ARGS(&adapter)),
          "failed to enum dxgi adapter");
  err_if(D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_12_1, IID_PPV_ARGS(&_device)),
          "failed to create d3d12 device");

  // get render target view descriptor size
  RTV_Size         = _device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
  CBV_SRV_UAV_Size = _device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
  DSV_Size         = _device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);

  err_if(DCompositionCreateDevice(nullptr, IID_PPV_ARGS(&_device_comp)),
        "failed to create composition device");

  create_device_11on12();
}

auto Core::create_cmd_alloc(D3D12_COMMAND_LIST_TYPE type) const noexcept -> Microsoft::WRL::ComPtr<ID3D12CommandAllocator>
{
  auto alloc = ComPtr<ID3D12CommandAllocator>{};
  err_if(_device->CreateCommandAllocator(type, IID_PPV_ARGS(&alloc)), "failed to create command allocator");
  return alloc;
}

auto Core::create_cmd(D3D12_COMMAND_LIST_TYPE type, ID3D12CommandAllocator* alloc) const noexcept -> Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList1>
{
  auto cmd = ComPtr<ID3D12GraphicsCommandList1>{};
  err_if(_device->CreateCommandList(0, type, alloc, nullptr, IID_PPV_ARGS(&cmd)),
          "failed to create command list");
  err_if(cmd->Close(), "failed to close command list");
  return cmd;
}

void Core::create_device_11on12() noexcept
{
  // create command queue
  auto queue_desc = D3D12_COMMAND_QUEUE_DESC{};
  queue_desc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
  err_if(_device->CreateCommandQueue(&queue_desc, IID_PPV_ARGS(&_cmd_queue)),
          "failed to create command queue");

  // create d3d11on12 device
  auto cmd_queue = _cmd_queue.Get();
  err_if(D3D11On12CreateDevice(_device.Get(), D3D11_CREATE_DEVICE_BGRA_SUPPORT, nullptr, 0,
    reinterpret_cast<IUnknown**>(&cmd_queue), 1, 0, &_device_11, &_device_11_ctx, nullptr),
    "failed to create d3d11on12");
  _device_11on12 = TryAs<ID3D11On12Device>(_device_11);
  err_if(!_device_11on12, "failed to get d3d11on12 device");

  // create dxgi device
  auto dxgi_device = TryAs<IDXGIDevice>(_device_11);
  err_if(!dxgi_device, "failed to get dxgi device");

  // create d2d device
  err_if(D2D1CreateDevice(dxgi_device.Get(), {}, &_device_d2d), "failed to create d2d device");
  _device_d2d->CreateDeviceContext(D2D1_DEVICE_CONTEXT_OPTIONS_NONE, &_device_d2d_ctx);
}

}
