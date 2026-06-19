#pragma once

#include <d3d12.h>

#include "image_manager.hpp"
#include "render_resource.hpp"

#include <initializer_list>
#include <unordered_set>
#include <unordered_map>

namespace tk::renderer {

struct BitmapCopyInfo
{
  BitmapView bitmap_view; 
  uint2      pos;
};

class Command
{
public:
  Command()                          = default;
  ~Command()                         = default;
  Command(Command const&)            = delete;
  Command(Command&&)                 = delete;
  Command& operator=(Command const&) = delete;
  Command& operator=(Command&&)      = delete;

  void init(D3D12_COMMAND_LIST_TYPE type, ID3D12CommandAllocator* alloc) noexcept;
  void destroy() noexcept;

  void reinit(ID3D12CommandAllocator* alloc) const noexcept;
  void close() const noexcept;

  auto get() const noexcept { return _cmd; }

  template <typename... Spans>
  void barrier(Spans&&... spans) const noexcept;

  void transform(ImageHandle image, Flag<ImageState> states, uint subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES) const noexcept;
  struct TransformInfo
  {
    ImageHandle      image;
    Flag<ImageState> states;
    uint             subresource{ D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES };
  };

  void transform(std::initializer_list<TransformInfo> infos) const noexcept;

  template <std::ranges::input_range R>
  requires std::convertible_to<std::ranges::range_value_t<R>, TransformInfo>
  void transform(R&& range) const noexcept;

  void clear(ImageHandle image, D3D12_CPU_DESCRIPTOR_HANDLE cpu_handle, D3D12_GPU_DESCRIPTOR_HANDLE gpu_handle) const noexcept;
  void clear_render_target(ImageHandle image, std::optional<Rect> rect = {}) noexcept;
  void clear_depth_stencil(ImageHandle image, std::optional<Rect> rect = {}) noexcept;

  void copy(ImageHandle src, Rect rect, ImageHandle dst, uint2 pos) noexcept;
  void copy(ImageHandle src, ImageHandle dst) noexcept;
  void copy(BufferHandle buf, ImageHandle img, std::span<BitmapCopyInfo const> bitmap_copy_infos) noexcept;
  void copy(BufferHandle buf, ImageHandle img, Bitmap const& bitmap) noexcept
  {
    auto infos = std::vector<BitmapCopyInfo>{{ bitmap.to_bitmap_view() }};
    copy(buf, img, infos);
  }

  void upload(FrameBuffer& buf, ui::FrameData const* data) noexcept;
  void bind_descriptor_heaps() const noexcept;
  
  auto needs_graphics_sync() const noexcept -> bool;
  auto needs_compute_sync()  const noexcept -> bool;
  auto needs_copy_sync()     const noexcept -> bool;

  void clear_resource_track() const noexcept;

private:
  ID3D12GraphicsCommandList1* _cmd{};
};

Singleton(CommandPool, g_cmd_pool,
  friend class Command;
public:
  using Pool   = ObjectPool<Command>;
  using Handle = Pool::Handle;

  auto create(D3D12_COMMAND_LIST_TYPE type, ID3D12CommandAllocator* alloc) noexcept
  {
    auto h = _pool.alloc();
    auto& cmd = _pool[h];
    cmd.init(type, alloc);
    _resources.emplace(cmd.get(), Resource{});
    return h;
  }

  void destroy(Handle& h) noexcept
  {
    auto& cmd = _pool[h];
    _resources.erase(cmd.get());
    cmd.destroy();
    _pool.free(h);
  }

  auto& operator[](Handle h) noexcept { return _pool[h]; }

  auto get(Handle h) noexcept { return &_pool[h]; }

  auto reinit(Handle h, ID3D12CommandAllocator* alloc) noexcept
  {
    auto& cmd = _pool[h];
    cmd.reinit(alloc);
    return &cmd;
  }

private:
  void clear_resource_track(Command const* cmd) noexcept
  {
    _resources[cmd->get()].clear();
  }

  struct Resource
  {
    std::unordered_set<ImageHandle>  imgs;
    std::unordered_set<BufferHandle> bufs;

    void clear() noexcept
    {
      for (auto img : imgs) g_img_mgr[img].reset_resource_track();
      for (auto buf : bufs) g_buf_pool[buf].reset_resource_track();
      imgs.clear();
      bufs.clear();
    }
  };
  auto resource(ID3D12GraphicsCommandList1* cmd) noexcept -> Resource&;

private:
  Pool _pool;
  std::unordered_map<ID3D12GraphicsCommandList1*, Resource> _resources;
)

using CmdHandle = CommandPool::Handle;

class CmdQueue
{
public:
  CmdQueue()                           = default;
  ~CmdQueue()                          = default;
  CmdQueue(CmdQueue const&)            = delete;
  CmdQueue(CmdQueue&&)                 = delete;
  CmdQueue& operator=(CmdQueue const&) = delete;
  CmdQueue& operator=(CmdQueue&&)      = delete;

  void init(D3D12_COMMAND_LIST_TYPE type) noexcept;

  void destroy() noexcept;

  void wait(ID3D12Fence* fence, uint64 value) const noexcept;
  void signal(ID3D12Fence* fence, uint64 value) const noexcept;
  void submit(ID3D12Fence* fence, uint64 value, std::initializer_list<CmdHandle> cmds) const noexcept;

  auto get() const noexcept { return _queue; }

private:
  ID3D12CommandQueue* _queue{};
};

template <typename... Spans>
void Command::barrier(Spans&&... spans) const noexcept
{
  auto size = (spans.size() + ...);
  if (!size) return;
  auto barriers = std::vector<D3D12_RESOURCE_BARRIER>{};
  barriers.reserve(size);
  (barriers.append_range(spans), ...);
  _cmd->ResourceBarrier(barriers.size(), barriers.data());
}

template <std::ranges::input_range R>
requires std::convertible_to<std::ranges::range_value_t<R>, Command::TransformInfo>
void Command::transform(R&& range) const noexcept
{
  auto barriers = std::vector<D3D12_RESOURCE_BARRIER>{};
  for (auto [image, states, subresource] : range)
  {
    barriers.append_range(g_img_mgr[image].transform(this, states, subresource));
    g_cmd_pool.resource(_cmd).imgs.emplace(image);
  };
  barrier(barriers);
}

}
