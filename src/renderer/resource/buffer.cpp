#include "buffer.hpp"
#include "util/error_handling.hpp"
#include "../core.hpp"
#include "../renderer.hpp"
#include "../../util/align.hpp"

#include <directx/d3dx12.h>

using namespace tk;
using namespace tk::renderer;

namespace {

auto calculate_capacity(uint old_capacity, uint need_capacity)
{
  auto factor = (old_capacity < 256 * 1024)      ? 2.0 :
                (old_capacity < 8 * 1024 * 1024) ? 1.5 : 1.25;

  auto capacity = static_cast<uint>(old_capacity * factor);
  if (old_capacity < need_capacity) capacity = need_capacity;

  // Round up to 256 bytes
  capacity = align(capacity, 256);

  // Clamp to max budget (optional)
  constexpr size_t Max = 128ull * 1024 * 1024;
  if (capacity > Max) capacity = align(need_capacity, 256);

  return capacity;
}

}

namespace tk::renderer {

void Buffer::init(uint size, bool use_descriptor) noexcept
{
  _size     = {};
  _capacity = align(size, 8);

  auto create_descriptor = [this]
  {
    D3D12_SHADER_RESOURCE_VIEW_DESC srv_desc{};
    srv_desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srv_desc.Format                  = DXGI_FORMAT_R32_TYPELESS;
    srv_desc.ViewDimension           = D3D12_SRV_DIMENSION_BUFFER;
    srv_desc.Buffer.Flags            = D3D12_BUFFER_SRV_FLAG_RAW;
    srv_desc.Buffer.NumElements      = _capacity / 4;
    g_core.device()->CreateShaderResourceView(_handle.Get(), &srv_desc, _descriptor_handle.cpu_handle());
  };

  if (use_descriptor && !_handle)
    _descriptor_handle = g_desc_heap_mgr.pop_handle(DescriptorHeapType::cbv_srv_uav, create_descriptor);
  
  // create buffer
  auto heap_properties = CD3DX12_HEAP_PROPERTIES{ D3D12_HEAP_TYPE_UPLOAD };
  auto resource_desc   = CD3DX12_RESOURCE_DESC::Buffer(_capacity);
  err_if(g_core.device()->CreateCommittedResource(
    &heap_properties,
    D3D12_HEAP_FLAG_NONE,
    &resource_desc,
    D3D12_RESOURCE_STATE_GENERIC_READ,
    nullptr,
    IID_PPV_ARGS(_handle.ReleaseAndGetAddressOf())),
    "failed to create vertex buffer");

  // get pointer of buffer
  auto range = CD3DX12_RANGE{};
  err_if(_handle->Map(0, &range, reinterpret_cast<void**>(&_data)), "failed to map pointer from buffer");

  if (use_descriptor)
    create_descriptor();
}

void Buffer::offset(uint size) noexcept
{
  assert(size <= _capacity);
  _size = size;
}

void Buffer::copy(void const* data, uint size) const noexcept
{
  size = align(size, 4);
  auto total_size = _size + size;
  assert(total_size <= _capacity);
  memcpy(_data + _size, data, size);
}

auto Buffer::append(void const* data, uint size) noexcept -> uint
{
  // promise aligment
  size = align(size, 4);
  auto total_size = _size + size;
  if (total_size <= _capacity)
  {
    memcpy(_data + _size, data, size);
    _size = total_size;
  }
  else
  {
    reserve(total_size);

    // copy current data again
    append(data, size);
  }
  return size;
}

void Buffer::resize(uint size) noexcept
{
  if (size <= _size) return;
  reserve(size);
  _size = size;
}

void Buffer::reserve(uint size) noexcept
{
  if (size <= _capacity) return;

  // add old buffer for destroy
  g_renderer.add_frame_render_complete_func([_ = _handle] {}, EngineType::graphics);

  // temporary copy old data
  auto old_data = std::vector<std::byte>(_size);
  memcpy(old_data.data(), _data, _size);

  // create new bigger one
  init(calculate_capacity(_capacity, size), _descriptor_handle.is_valid());

  // copy old data to new buffer
  append(old_data.data(), old_data.size());
}

}
