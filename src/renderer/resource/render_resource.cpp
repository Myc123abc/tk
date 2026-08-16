#include "render_resource.hpp"
#include "../core.hpp"
#include "../engine/graphics_engine.hpp"
#include "tk/error_handling.hpp"
#include "command.hpp"

#include <directx/d3dx12.h>
#include <windows.h>

#include <ranges>

using namespace Microsoft::WRL;

namespace tk::renderer {

void RenderResource::init(HWND handle, uint width, uint height) noexcept
{
  // create images
  for (auto& frame : _frames)
    frame.image = g_img_mgr.create(width, height, Render_Target_Format, ImageType::rtv);
  _dsv_image  = g_img_mgr.create(width, height, ImageFormat::d24_s8, ImageType::dsv);

  // create swapchain
  ComPtr<IDXGISwapChain1> swapchain;
  DXGI_SWAP_CHAIN_DESC1 swapchain_desc{};
  swapchain_desc.BufferCount      = Frame_Count;
  swapchain_desc.Width            = width;
  swapchain_desc.Height           = height;
  swapchain_desc.Format           = static_cast<DXGI_FORMAT>(Render_Target_Format);
  swapchain_desc.BufferUsage      = DXGI_USAGE_RENDER_TARGET_OUTPUT;
  swapchain_desc.SwapEffect       = DXGI_SWAP_EFFECT_FLIP_DISCARD;
  swapchain_desc.SampleDesc.Count = 1;
  swapchain_desc.Flags            = DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT | DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING;
  swapchain_desc.AlphaMode        = DXGI_ALPHA_MODE_PREMULTIPLIED;
  err_if(g_core.factory()->CreateSwapChainForComposition(g_graphics_engine.queue()->get(), &swapchain_desc, nullptr, &swapchain),
          "failed to create swapchain for composition");

  // create composition
  err_if(g_core.device_comp()->CreateTargetForHwnd(handle, true, &_comp_target),
          "failed to create composition target");
  err_if(g_core.device_comp()->CreateVisual(&_comp_visual),
          "failed to create composition visual");
  err_if(_comp_visual->SetContent(swapchain.Get()),
          "failed to bind swapchain to composition visual");
  err_if(_comp_target->SetRoot(_comp_visual.Get()),
          "failed to bind composition visual to target");
  err_if(g_core.device_comp()->Commit(),
          "failed to commit composition device");

  // set swapchain property and get waitable object
  _swapchain = TryAs<IDXGISwapChain4>(swapchain);
  err_if(!_swapchain, "failed to get swapchain4");
  _swapchain->SetMaximumFrameLatency(Frame_Count);
  _swapchain_waitable_obj = _swapchain->GetFrameLatencyWaitableObject();
  err_if(!_swapchain_waitable_obj, "failed to get waitable object from swapchain");

  // get image from swapchain backbuffers
  for (auto [i, frame] : _frames | std::views::enumerate)
    frame.swapchain_image = g_img_mgr.create(_swapchain.Get(), i);

  // create command allocator and list
  for (auto& frame : _frames)
  {
		frame.cmd_alloc = g_core.create_cmd_alloc(D3D12_COMMAND_LIST_TYPE_DIRECT);

    // initialize frame buffer
    frame.buffer.init();
  }
}

void RenderResource::destroy() noexcept
{
  CloseHandle(_swapchain_waitable_obj);
  g_img_mgr.destroy(_dsv_image);
  for (auto& frame : _frames)
  {
    g_img_mgr.destroy(frame.image);
    g_img_mgr.destroy(frame.swapchain_image);
    frame.buffer.destroy();
  }
}

void RenderResource::resize(uint width, uint height) noexcept
{
  // wait gpu complete
  g_graphics_engine.signal();
  WaitForSingleObjectEx(g_graphics_engine.set_event_on_completion(), INFINITE, false);

  // reset swapchain relatation resources
  for (auto& frame : _frames)
  {
    g_img_mgr[frame.image].destroy();;
    g_img_mgr[frame.swapchain_image].destroy();
  }
  _comp_visual->SetContent(nullptr);
  g_img_mgr[_dsv_image].destroy();

  // resize swapchain
  err_if(_swapchain->ResizeBuffers(Frame_Count, width, height, DXGI_FORMAT_UNKNOWN, DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT | DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING),
          "failed to resize swapchain");

  // rebind composition resources
  err_if(_comp_visual->SetContent(_swapchain.Get()),
          "failed to bind swapchain to composition visual");
  err_if(g_core.device_comp()->Commit(),
          "failed to commit composition device");

  // recreate images
  for (auto [i, frame] : _frames | std::views::enumerate)
  {
    g_img_mgr[frame.swapchain_image].resize(_swapchain.Get(), i);
    g_img_mgr[frame.image].resize(width, height);
  }
  g_img_mgr[_dsv_image].resize(width, height);
}

void RenderResource::wait_frame_complete() noexcept
{
  auto& frame = _frames[_frame_index];
  if (g_graphics_engine.fence_completed_value() < frame.fence_value)
  {
    auto objs = std::array<HANDLE, 2>
    {
      g_graphics_engine.set_event_on_completion(),
      _swapchain_waitable_obj
    };
    WaitForMultipleObjects(objs.size(), objs.data(), true, INFINITE);
  }
  else
    WaitForSingleObjectEx(_swapchain_waitable_obj, INFINITE, false);
}

void RenderResource::render_begin() noexcept
{
  auto& frame = current_frame();
  auto  cmd   = g_graphics_engine.reset_cmd(frame.cmd_alloc.Get());

  // bind heaps
  g_desc_heap_mgr.bind_heaps(cmd);

  cmd->clear_render_target(render_target());
}

void RenderResource::render_end() noexcept
{
	auto& frame           = current_frame();
  auto  swapchain_image = _frames[_swapchain->GetCurrentBackBufferIndex()].swapchain_image;
  auto  cmd             = g_graphics_engine.cmd();

  // copy offscreen image to swapchain backbuffer
	cmd->copy(frame.image, swapchain_image);

  // set to present state
  cmd->transform(swapchain_image, ImageState::present);

	// submit graphics commands to graphics engine
	frame.fence_value = g_sub_tracker.submit(g_graphics_engine);

  // move to next frame
  _frame_index = (_frame_index + 1) % Frame_Count;
}

void RenderResource::present(bool vsync) const noexcept
{
  auto res = HRESULT{};
  vsync
    ? res = _swapchain->Present(1, 0)
    : res = _swapchain->Present(0, DXGI_PRESENT_ALLOW_TEARING);
  if (FAILED(res))
  {
    res = g_core.device()->GetDeviceRemovedReason();
    err_if(res == DXGI_ERROR_DEVICE_HUNG, "failed to present, device hung");
    err_if(true, "failed to present : {}", static_cast<uint>(res));
  }
}

void FrameBuffer::init() noexcept
{
  _vertices_indices_buffer = g_buf_pool.create(Buffer_Init_Size, false);
}

void FrameBuffer::upload(Command const* cmd, ui::FrameData const* data) noexcept
{
  auto& buf = g_buf_pool[_vertices_indices_buffer];

  auto vertices_offset = buf.append_range(data->vertices());
  auto indices_offset  = buf.append_range(data->indices());

  // get current buffer gpu address
  auto address = buf.gpu_address();

  // set vertex buffer view
  D3D12_VERTEX_BUFFER_VIEW vertex_buffer_view{};
  vertex_buffer_view.BufferLocation = address;
  vertex_buffer_view.StrideInBytes  = sizeof(Vertex);
  vertex_buffer_view.SizeInBytes    = vertices_offset;
  cmd->get()->IASetVertexBuffers(0, 1, &vertex_buffer_view);

  // add vertices offset
  address += vertices_offset;

  // set index buffer view
  D3D12_INDEX_BUFFER_VIEW index_buffer_view{};
  index_buffer_view.BufferLocation = address;
  index_buffer_view.SizeInBytes    = indices_offset;
  index_buffer_view.Format         = DXGI_FORMAT_R16_UINT;
  cmd->get()->IASetIndexBuffer(&index_buffer_view);
}

}
