#include "descriptor_heap_manager.hpp"
#include "util/error_handling.hpp"
#include "../core.hpp"
#include "../renderer.hpp"
#include "../config.hpp"

#include <algorithm>
#include <ranges>
#include <unordered_map>

using namespace tk;
using namespace tk::renderer;
using namespace Microsoft::WRL;

namespace {

auto dx12_descriptor_size(DescriptorHeapType type) noexcept
{
  using enum DescriptorHeapType;
  auto static map = std::unordered_map<DescriptorHeapType, uint32_t>
  {
    { cbv_srv_uav, CBV_SRV_UAV_Size },
    { rtv,         RTV_Size         },
    { dsv,         DSV_Size         },
  };
  err_if(!map.contains(type), "unsupport descriptor size now");
  return map[type];
}

}

namespace tk::renderer {

void DescriptorHandle::release() noexcept
{
  if (is_valid())
  {
    auto& slot = g_desc_heap_mgr._heaps[_type]._handles[_index];
    slot.used                = false;
    slot.recreate_descriptor = {};
    _index = -1;
  }
}

auto DescriptorHandle::cpu_handle() const noexcept -> D3D12_CPU_DESCRIPTOR_HANDLE
{
  auto handle = g_desc_heap_mgr._heaps[_type]._heap->GetCPUDescriptorHandleForHeapStart();
  handle.ptr += dx12_descriptor_size(_type) * _index;
  return handle;
}

auto DescriptorHandle::gpu_handle() const noexcept -> D3D12_GPU_DESCRIPTOR_HANDLE
{
  auto handle = g_desc_heap_mgr._heaps[_type]._heap->GetGPUDescriptorHandleForHeapStart();
  handle.ptr += dx12_descriptor_size(_type) * _index;
  return handle;
}

void DescriptorHeapManager::DescriptorHeap::init(DescriptorHeapType type, uint32_t capacity) noexcept
{
  _type = type;

  // create descriptor heap
  auto heap_desc = D3D12_DESCRIPTOR_HEAP_DESC{};
  heap_desc.NumDescriptors = capacity;
  heap_desc.Type           = static_cast<D3D12_DESCRIPTOR_HEAP_TYPE>(type);
  heap_desc.Flags          = type == DescriptorHeapType::cbv_srv_uav ? D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE : D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
  err_if(g_core.device()->CreateDescriptorHeap(&heap_desc, IID_PPV_ARGS(&_heap)), "failed to create descriptor heap");

  // initialize handles
  _handles.resize(capacity);
}

auto DescriptorHeapManager::DescriptorHeap::pop_handle(std::function<void()> recreate_descriptor_func) noexcept -> DescriptorHandle
{
  // find a not used handle
  auto it = std::ranges::find_if_not(_handles, [](auto const& slot) { return slot.used; });

  // if not found, the heap is full, expand it
  if (it == _handles.end())
  {
    reserve(std::max<size_t>(_handles.size() * 2, 1));
    it = std::ranges::find_if_not(_handles, [](auto const& slot) { return slot.used; });
  }

  // find a useful handle
  it->used                = true;
  it->handle._type        = _type;
  it->handle._index       = it - _handles.begin();
  it->recreate_descriptor = std::move(recreate_descriptor_func);
  return it->handle;
}

void DescriptorHeapManager::DescriptorHeap::reserve(uint32_t capacity) noexcept
{
  if (capacity > _handles.size())
  {
    auto size = _handles.size();

    // destroy old heap
    g_renderer.add_frame_render_complete_func([_ = _heap] {});

    // create new bigger one
    auto heap_desc = D3D12_DESCRIPTOR_HEAP_DESC{};
    heap_desc.NumDescriptors = capacity;
    heap_desc.Type           = static_cast<D3D12_DESCRIPTOR_HEAP_TYPE>(_type);
    heap_desc.Flags          = _type == DescriptorHeapType::cbv_srv_uav ? D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE : D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    err_if(g_core.device()->CreateDescriptorHeap(&heap_desc, IID_PPV_ARGS(&_heap)), "failed to create descriptor heap");
    _handles.resize(capacity);

    // recreate descriptors
    std::ranges::for_each(_handles | std::views::take(size),
      [](auto const& slot) { if (slot.used && slot.recreate_descriptor) slot.recreate_descriptor(); });
  }
}

auto DescriptorHeapManager::DescriptorHeap::usable_handle_count() const noexcept -> uint32_t
{
  return std::ranges::count_if(_handles, [](auto const& slot) { return slot.used; });
}

void DescriptorHeapManager::init() noexcept
{
  using enum DescriptorHeapType;
  _heaps[cbv_srv_uav].init(cbv_srv_uav, CBV_SRV_UAV_Heap_Size);
  _heaps[rtv].init(rtv, RTV_Heap_Size);
  _heaps[dsv].init(dsv, DSV_Size);
}

void DescriptorHeapManager::bind_heaps(ID3D12GraphicsCommandList1* cmd) noexcept
{
  auto descriptor_heaps = std::array<ID3D12DescriptorHeap*, 1>{ _heaps[DescriptorHeapType::cbv_srv_uav]._heap.Get() };
  cmd->SetDescriptorHeaps(descriptor_heaps.size(), descriptor_heaps.data());
}

}
