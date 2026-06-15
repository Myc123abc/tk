#pragma once

#include "descriptor_heap_manager.hpp"
#include "../../util/object_pool.hpp"
#include "resource_tack.hpp"

#include <d3d12.h>
#include <wrl/client.h>

namespace tk::renderer {

class Buffer : public ResourceTrack
{
public:
  Buffer()                         = default;
  ~Buffer()                        = default;
  Buffer(Buffer const&)            = delete;
  Buffer(Buffer&&)                 = delete;
  Buffer& operator=(Buffer const&) = delete;
  Buffer& operator=(Buffer&&)      = delete;

  void init(uint size, bool use_descriptor) noexcept;

  void destroy() noexcept { _descriptor_handle.release(); }

  void clear() noexcept { _size = {}; }

  auto append(void const* data, uint size) noexcept -> uint;

  template <std::ranges::range T>
  requires std::ranges::sized_range<T> && std::ranges::contiguous_range<T>
  auto append_range(T&& values) noexcept -> uint
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
  uint8*                                 _data{};
  uint                                   _capacity{};
  uint                                   _size{};
};

Singleton(BufferdPool, g_buf_pool,
public:
  using Pool   = ObjectPool<Buffer>;
  using Handle = Pool::Handle;

  auto create(uint size, bool use_descriptor) noexcept
  {
    auto h = _pool.alloc();
    _pool[h].init(size, use_descriptor);
    return h;
  }

  void destroy(Handle& h) noexcept
  {
    _pool[h].destroy();
    _pool.free(h);
  }

  auto& operator[](Handle h) noexcept { return _pool[h]; }

private:
  Pool _pool;
)

using BufferHandle = BufferdPool::Handle;

}
