#include "command.hpp"
#include "util/error_handling.hpp"
#include "../core.hpp"
#include "../../util/align.hpp"
#include "descriptor_heap_manager.hpp"

#include <ranges>

namespace tk::renderer {

void Command::init(D3D12_COMMAND_LIST_TYPE type, ID3D12CommandAllocator* alloc) noexcept
{
  err_if(g_core.device()->CreateCommandList(0, type, alloc, nullptr, IID_PPV_ARGS(&_cmd)),
          "failed to create command list");
  err_if(_cmd->Close(), "failed to close command list");
}

void Command::destroy() noexcept
{
  _cmd->Release();
  _cmd = {};
}

void Command::reinit(ID3D12CommandAllocator* alloc) const noexcept
{
  err_if(alloc->Reset() == E_FAIL, "failed to reset command allocator");
  err_if(_cmd->Reset(alloc, nullptr), "failed to reset command list");
}

void Command::close() const noexcept
{
  err_if(_cmd->Close(), "failed to close command list");
}

void CmdQueue::init(D3D12_COMMAND_LIST_TYPE type) noexcept
{
  auto queue_desc = D3D12_COMMAND_QUEUE_DESC{};
  queue_desc.Type = type;
  err_if(g_core.device()->CreateCommandQueue(&queue_desc, IID_PPV_ARGS(&_queue)),
        "failed to create command queue");
}

void CmdQueue::destroy() noexcept
{
  _queue->Release();
  _queue = {};
}

void CmdQueue::signal(ID3D12Fence* fence, uint64 value) const noexcept
{
  err_if(_queue->Signal(fence, value), "failed to signal fence");
}

void CmdQueue::submit(ID3D12Fence* fence, uint64 value, std::initializer_list<CmdHandle> cmd_hs) const noexcept
{
  // get commands
  auto cmds = cmd_hs
    | std::views::transform([](auto h) { return g_cmd_pool.get(h); })
    | std::ranges::to<std::vector<Command*>>();

  // close commands
  for (auto cmd : cmds) cmd->close();

  // execute command lists
  auto cmd_lists = cmds
    | std::views::transform([](auto cmd) { return cmd->get(); })
    | std::ranges::to<std::vector<ID3D12GraphicsCommandList1*>>();
  _queue->ExecuteCommandLists(cmd_lists.size(), reinterpret_cast<ID3D12CommandList* const*>(cmd_lists.data()));

  signal(fence, value);

  // TODO: cmd store used resources, set fence value on these resources' track state
}

void CmdQueue::wait(ID3D12Fence* fence, uint64 value) const noexcept
{
  _queue->Wait(fence, value);
}

auto CommandPool::resource(ID3D12GraphicsCommandList1* cmd) noexcept -> Resource&
{
  assert(_resources.contains(cmd));
  return _resources[cmd];
}

////////////////////////////////////////////////////////////////////////////////
///                          command operations
////////////////////////////////////////////////////////////////////////////////

void Command::transform(ImageHandle image, ImageState state, uint subresource) const noexcept
{
  g_img_mgr[image].transform(this, state, subresource);
  g_cmd_pool.resource(_cmd).imgs.emplace(image);
}

void Command::clear(ImageHandle image, D3D12_CPU_DESCRIPTOR_HANDLE cpu_handle, D3D12_GPU_DESCRIPTOR_HANDLE gpu_handle) const noexcept
{
  g_img_mgr[image].clear(this, cpu_handle, gpu_handle);
  g_cmd_pool.resource(_cmd).imgs.emplace(image);
}

void Command::clear_render_target(ImageHandle image, std::optional<Rect> rect) noexcept
{
  g_img_mgr[image].clear_render_target(this, rect);
  g_cmd_pool.resource(_cmd).imgs.emplace(image);
}

void Command::clear_depth_stencil(ImageHandle image, std::optional<Rect> rect) noexcept
{
  g_img_mgr[image].clear_depth_stencil(this, rect);
  g_cmd_pool.resource(_cmd).imgs.emplace(image);
}

void Command::copy(ImageHandle src, Rect rect, ImageHandle dst, uint2 pos) noexcept
{
  auto& src_img = g_img_mgr[src];
  auto& dst_img = g_img_mgr[dst];

  src_img.transform(this, ImageState::copy_src);
  dst_img.transform(this, ImageState::copy_dst);

  auto src_loc = CD3DX12_TEXTURE_COPY_LOCATION{ src_img.handle() };
  auto dst_loc = CD3DX12_TEXTURE_COPY_LOCATION{ dst_img.handle() };

  auto region_box = CD3DX12_BOX
  {
    static_cast<LONG>(rect.left),
    static_cast<LONG>(rect.top),
    static_cast<LONG>(rect.right),
    static_cast<LONG>(rect.bottom)
  };
  _cmd->CopyTextureRegion(&dst_loc, pos.x, pos.y, 0, &src_loc, &region_box);

  g_cmd_pool.resource(_cmd).imgs.emplace(src);
  g_cmd_pool.resource(_cmd).imgs.emplace(dst);
}

void Command::copy(ImageHandle src, ImageHandle dst) noexcept
{
  copy(src, { 0, 0, g_img_mgr[src].extent() }, dst, {});
}

void Command::copy(ImageHandle image_h, BufferHandle upload_heap, uint offset, D3D12_SUBRESOURCE_DATA& data) noexcept
{
  auto& img = g_img_mgr[image_h];

  img.transform(this, ImageState::copy_dst);
  UpdateSubresources(_cmd, img.handle(), g_buf_pool[upload_heap].handle(), offset, 0, 1, &data);

  g_cmd_pool.resource(_cmd).imgs.emplace(image_h);
  g_cmd_pool.resource(_cmd).bufs.emplace(upload_heap);
}

void Command::copy(BufferHandle src, ImageHandle dst, uint src_offset, BitmapView const& bitmap, uint2 pos) noexcept
{
  auto& buf = g_buf_pool[src];
  auto& img = g_img_mgr[dst];

  img.transform(this, ImageState::copy_dst);

  auto footprint = D3D12_PLACED_SUBRESOURCE_FOOTPRINT{};
  footprint.Offset             = src_offset;
  footprint.Footprint.Width    = bitmap.width;
  footprint.Footprint.Height   = bitmap.height;
  footprint.Footprint.Depth    = 1;
  footprint.Footprint.RowPitch = align(bitmap.row_pitch, D3D12_TEXTURE_DATA_PITCH_ALIGNMENT);
  footprint.Footprint.Format   = static_cast<DXGI_FORMAT>(img.format());

  auto src_loc = CD3DX12_TEXTURE_COPY_LOCATION{ buf.handle(), footprint };
  auto dst_loc = CD3DX12_TEXTURE_COPY_LOCATION{ img.handle() };

  _cmd->CopyTextureRegion(&dst_loc, pos.x, pos.y, 0, &src_loc, nullptr);

  g_cmd_pool.resource(_cmd).bufs.emplace(src);
  g_cmd_pool.resource(_cmd).imgs.emplace(dst);
}

void Command::upload(FrameBuffer& buf, ui::FrameData const* data) noexcept
{
  buf.upload(this, data);
  g_cmd_pool.resource(_cmd).bufs.emplace(buf.vertice_indices_buf_handle());
}

void Command::bind_descriptor_heaps() const noexcept
{
  g_desc_heap_mgr.bind_heaps(this);
}

// void copy(Bitmap const& src, Bitmap const& dst) noexcept
// {
//   assert(src.channel == dst.channel);
//   auto src_data = reinterpret_cast<std::byte*>(src.data);
//   auto dst_data = reinterpret_cast<std::byte*>(dst.data);
//   for (auto i = 0; i < dst.height; ++i)
//   {
//     memcpy(dst_data, src_data, src.width * src.channel);
//     src_data += src.row_pitch;
//     dst_data += dst.row_pitch;
//   }
// }


}
