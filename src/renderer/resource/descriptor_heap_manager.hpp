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
  auto cpu_handle() const noexcept -> D3D12_CPU_DESCRIPTOR_HANDLE;
  auto gpu_handle() const noexcept -> D3D12_GPU_DESCRIPTOR_HANDLE;

  void release() noexcept;

  auto is_valid() const noexcept { return _index >= 0; }

  auto index() const noexcept { return _index; }

  void set(std::function<void()> func) noexcept { _recreate_descriptor_func = func; }

private:
  int                   _index{ -1 };
  DescriptorHeapType    _type{};
  std::function<void()> _recreate_descriptor_func; // TODO: resdesign the recreate descriptor way
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

    void init(DescriptorHeapType type, uint32_t capacity) noexcept;

    auto pop_handle(std::function<void()> recreate_descriptor_func) noexcept -> DescriptorHandle;

    void reserve(uint32_t capacity) noexcept;

    auto usable_handle_count() const noexcept -> uint32_t;

    auto size() const noexcept { return _handles.size(); }

  private:
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>   _heap;
    std::vector<std::pair<bool, DescriptorHandle>> _handles;
    DescriptorHeapType                             _type{};
  };

public:
  void init() noexcept;

  auto pop_handle(DescriptorHeapType type, std::function<void()> recreate_descriptor_func = {}) noexcept { return _heaps[type].pop_handle(recreate_descriptor_func); }

  void bind_heaps(ID3D12GraphicsCommandList1* cmd) noexcept;

  void reserve(DescriptorHeapType type, uint32_t capacity) noexcept { _heaps[type].reserve(capacity); }

  auto size(DescriptorHeapType type) noexcept { return _heaps[type].size(); }

  auto usable_handle_count(DescriptorHeapType type) noexcept { return _heaps[type].usable_handle_count(); }

  auto first_gpu_handle(DescriptorHeapType type) noexcept { return _heaps[type]._heap->GetGPUDescriptorHandleForHeapStart(); }

private:
  std::unordered_map<DescriptorHeapType, DescriptorHeap> _heaps;
)

}
