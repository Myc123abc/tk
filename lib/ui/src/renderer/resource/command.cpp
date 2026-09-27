#include "command.hpp"
#include "tk/error_handling.hpp"
#include "../core.hpp"
#include "../../util/align.hpp"
#include "descriptor_heap_manager.hpp"
#include "tk/base.hpp"

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

void CmdQueue::submit(ID3D12Fence* fence, uint64 value, std::span<Command*> cmds) const noexcept
{
  // close commands
  for (auto cmd : cmds) cmd->close();

  // execute command lists
  auto cmd_lists = cmds
    | std::views::transform([](auto cmd) { return cmd->get(); })
    | std::ranges::to<std::vector<ID3D12GraphicsCommandList1*>>();
  _queue->ExecuteCommandLists(cmd_lists.size(), reinterpret_cast<ID3D12CommandList* const*>(cmd_lists.data()));

  signal(fence, value);
}

void CmdQueue::wait(ID3D12Fence* fence, uint64 value) const noexcept
{
  _queue->Wait(fence, value);
}

////////////////////////////////////////////////////////////////////////////////
///                          command operations
////////////////////////////////////////////////////////////////////////////////

void Command::transform(std::initializer_list<TransformInfo> infos) const noexcept
{
  auto barriers = std::vector<D3D12_RESOURCE_BARRIER>{};
  for (auto const& [image, states, subresource] : infos)
  {
    auto img = g_img_mgr.get(image);
    auto res = img->transform(this, states, subresource);
    use(img, resource_access(states));
    barriers.append_range(std::move(res));
  };
  barrier(barriers);
}

void Command::transform(ImageHandle image, Flag<ImageState> states, uint subresource) const noexcept
{
  auto img = g_img_mgr.get(image);
  auto res = img->transform(this, states, subresource);
  use(img, resource_access(states));
  barrier(res);
}

void Command::clear(ImageHandle image, D3D12_CPU_DESCRIPTOR_HANDLE cpu_handle, D3D12_GPU_DESCRIPTOR_HANDLE gpu_handle) const noexcept
{
  auto img = g_img_mgr.get(image);
  img->clear(this, cpu_handle, gpu_handle);
  use(img, GPUResourceAccess::write);
}

void Command::clear_render_target(ImageHandle image, std::optional<Rect> rect) noexcept
{
  auto img = g_img_mgr.get(image);
  img->clear_render_target(this, rect);
  use(img, GPUResourceAccess::write);
}

void Command::clear_depth_stencil(ImageHandle image, std::optional<Rect> rect) noexcept
{
  auto img = g_img_mgr.get(image);
  img->clear_depth_stencil(this, rect);
  use(img, GPUResourceAccess::write);
}

void Command::copy(ImageHandle src, Rect rect, ImageHandle dst, uint2 pos) noexcept
{
  auto& src_img = g_img_mgr[src];
  auto& dst_img = g_img_mgr[dst];

  transform({
    { src, ImageState::copy_src },
    { dst, ImageState::copy_dst },
  });

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

  use(&src_img, GPUResourceAccess::read);
  use(&dst_img, GPUResourceAccess::write);
}

void Command::copy(ImageHandle src, ImageHandle dst) noexcept
{
  copy(src, { 0, 0, g_img_mgr[src].extent() }, dst, {});
}

void Command::copy(BufferHandle buf_h, ImageHandle img_h, std::span<BitmapCopyInfo const> bitmap_copy_infos) noexcept
{
  auto& buf = g_buf_pool[buf_h];
  auto& img = g_img_mgr[img_h];

  // transform image state
  transform(img_h, ImageState::copy_dst);

  for (auto const& [view, pos] : bitmap_copy_infos)
  {
    auto row_pitch = align(view.row_pitch, D3D12_TEXTURE_DATA_PITCH_ALIGNMENT);
    auto data      = reinterpret_cast<uint8 const*>(view.data);
    auto offset    = align(buf.size(), D3D12_TEXTURE_DATA_PLACEMENT_ALIGNMENT);

    // promise placement aligment for CopyTextureRegion
    buf.offset(offset);

    // copy bitmap data to buffer
    for (auto i = 0; i < view.height; ++i, data += view.row_pitch)
    {
      buf.copy(data, view.row_pitch);
      buf.offset(buf.size() + row_pitch);
    }

    // set buffer footprint
    auto footprint = D3D12_PLACED_SUBRESOURCE_FOOTPRINT{};
    footprint.Offset             = offset;
    footprint.Footprint.Width    = view.width;
    footprint.Footprint.Height   = view.height;
    footprint.Footprint.Depth    = 1;
    footprint.Footprint.RowPitch = row_pitch;
    footprint.Footprint.Format   = static_cast<DXGI_FORMAT>(img.format());

    auto src_loc = CD3DX12_TEXTURE_COPY_LOCATION{ buf.handle(), footprint };
    auto dst_loc = CD3DX12_TEXTURE_COPY_LOCATION{ img.handle() };

    // copy buffer data to texture
    _cmd->CopyTextureRegion(&dst_loc, pos.x, pos.y, 0, &src_loc, nullptr);
  }

  use(&img, GPUResourceAccess::write);
  use(&buf, GPUResourceAccess::read);
}

void Command::upload(FrameBuffer& buf, ui::FrameData const* data) noexcept
{
  buf.upload(this, data);
  use(&g_buf_pool[buf.vertice_indices_buf_handle()], GPUResourceAccess::read);
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
