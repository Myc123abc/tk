#pragma once

#include "../../util/singleton.hpp"
#include "util/base.hpp"

#include <d3d12.h>
#include <wrl/client.h>

#include <vector>
#include <unordered_map>
#include <functional>

namespace tk::renderer {

inline auto RTV_Size         = 0u;
inline auto CBV_SRV_UAV_Size = 0u;
inline auto DSV_Size         = 0u;

enum class DescriptorHeapType
{
  cbv_srv_uav = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
  rtv         = D3D12_DESCRIPTOR_HEAP_TYPE_RTV,
  dsv         = D3D12_DESCRIPTOR_HEAP_TYPE_DSV,
};

class DescriptorHandle
{
  friend class DescriptorHeapManager;
public:
  DescriptorHandle()                                   = default;
  ~DescriptorHandle()                                  = default;
  DescriptorHandle(DescriptorHandle const&)            = default;
  DescriptorHandle& operator=(DescriptorHandle const&) = default;

  DescriptorHandle(DescriptorHandle&& desc) noexcept { *this = std::move(desc); }
  DescriptorHandle& operator=(DescriptorHandle&& desc) noexcept
  {
    _index = desc._index;
    _type  = desc._type;
    desc._index = -1;
    return *this;
  }

  auto cpu_handle() const noexcept -> D3D12_CPU_DESCRIPTOR_HANDLE;
  auto gpu_handle() const noexcept -> D3D12_GPU_DESCRIPTOR_HANDLE;

  void release() noexcept;

  auto is_valid() const noexcept { return _index >= 0; }

  auto index() const noexcept { return _index; }

private:
  int                _index{ -1 };
  DescriptorHeapType _type{};
};

Singleton(DescriptorHeapManager, g_desc_heap_mgr,
  friend class DescriptorHandle;
public:
  class DescriptorHeap
  {
    friend class DescriptorHandle;
    friend class DescriptorHeapManager;
  public:
    DescriptorHeap()                                 = default;
    ~DescriptorHeap()                                = default;
    DescriptorHeap(DescriptorHeap const&)            = delete;
    DescriptorHeap(DescriptorHeap&&)                 = delete;
    DescriptorHeap& operator=(DescriptorHeap const&) = delete;
    DescriptorHeap& operator=(DescriptorHeap&&)      = delete;

    void init(DescriptorHeapType type, uint capacity) noexcept;

    auto pop_handle(std::function<void()> recreate_descriptor_func) noexcept -> DescriptorHandle;

    void reserve(uint capacity) noexcept;

    auto usable_handle_count() const noexcept -> uint;

    auto size() const noexcept { return _handles.size(); }
    
    void set(DescriptorHandle handle, std::function<void()> recreate_descriptor_func) noexcept;

  private:
    struct DescriptorSlot
    {
      bool                  used{};
      DescriptorHandle      handle{};
      std::function<void()> recreate_descriptor{};
    };

    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> _heap;
    std::vector<DescriptorSlot>                  _handles;
    DescriptorHeapType                           _type{};
  };

public:
  void init() noexcept;

  auto pop_handle(DescriptorHeapType type, std::function<void()> recreate_descriptor_func = {}) noexcept { return _heaps[type].pop_handle(recreate_descriptor_func); }

  void bind_heaps(ID3D12GraphicsCommandList1* cmd) noexcept;

  void reserve(DescriptorHeapType type, uint capacity) noexcept { _heaps[type].reserve(capacity); }

  auto size(DescriptorHeapType type) noexcept { return _heaps[type].size(); }

  auto usable_handle_count(DescriptorHeapType type) noexcept { return _heaps[type].usable_handle_count(); }

  auto first_gpu_handle(DescriptorHeapType type) noexcept { return _heaps[type]._heap->GetGPUDescriptorHandleForHeapStart(); }

  void set(DescriptorHandle handle, std::function<void()> recreate_descriptor_func) noexcept { _heaps[handle._type].set(handle, recreate_descriptor_func); }

private:
  std::unordered_map<DescriptorHeapType, DescriptorHeap> _heaps;
)

}
