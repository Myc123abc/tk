#pragma once

#include "descriptor_heap_manager.hpp"

#include <d3d12.h>
#include <wrl/client.h>

namespace tk::renderer {

class Buffer
{
public:
  void init(uint32_t size, bool use_descriptor) noexcept;

  void destroy() noexcept { _descriptor_handle.release(); }

  void clear() noexcept { _size = {}; }

  auto append(void const* data, uint32_t size) noexcept -> uint32_t;

  template <std::ranges::range T>
  requires std::ranges::sized_range<T> && std::ranges::contiguous_range<T>
  auto append_range(T&& values) noexcept -> uint32_t
  {
    return append(std::ranges::data(values), std::ranges::size(values) * sizeof(std::ranges::range_value_t<T>));
  }

  auto gpu_address() const noexcept { return _handle->GetGPUVirtualAddress(); }
  auto size()        const noexcept { return _size;                           }
  auto capacity()    const noexcept { return _capacity;                       }
  auto gpu_handle()  const noexcept { return _descriptor_handle.gpu_handle(); }
  auto handle()      const noexcept { return _handle.Get();                   }

private:
  Microsoft::WRL::ComPtr<ID3D12Resource> _handle;
  DescriptorHandle                       _descriptor_handle;
  uint8_t*                               _data{};
  uint32_t                               _capacity{};
  uint32_t                               _size{};
};

}
