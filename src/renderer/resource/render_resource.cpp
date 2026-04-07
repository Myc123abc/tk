#include "render_resource.hpp"
#include "../core.hpp"
#include "../engine/graphics_engine.hpp"
#include "util/error_handling.hpp"

#include <directx/d3dx12.h>
#include <windows.h>

#include <ranges>

using namespace Microsoft::WRL;

namespace tk::renderer {

void RenderResource::init(HWND handle, uint32_t width, uint32_t height) noexcept
{
  // create offscreen images
  for (auto& frame : _frames)
    frame.image.init(width, height, Render_Target_Format, ImageType::rtv);
  
  // create depth test image
  if (Enable_Depth_Test)
    _dsv_image.init(width, height, ImageFormat::d32, ImageType::dsv);

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
  err_if(g_core.factory()->CreateSwapChainForComposition(g_graphics_engine.queue(), &swapchain_desc, nullptr, &swapchain),
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
    frame.swapchain_image.init(_swapchain.Get(), i);

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
  if (Enable_Depth_Test)
    _dsv_image.destroy();
  for (auto& frame : _frames)
  {
    frame.image.destroy();
    frame.swapchain_image.destroy();
    frame.buffer.destroy();
  }
}

void RenderResource::resize(uint32_t width, uint32_t height) noexcept
{
  // wait gpu complete
  g_graphics_engine.signal();
  WaitForSingleObjectEx(g_graphics_engine.set_event_on_completion(), INFINITE, false);

  // reset swapchain relatation resources
  for (auto& frame : _frames)
  {
    frame.swapchain_image.destroy();
    frame.image.destroy();
  }
  _comp_visual->SetContent(nullptr);
  if (Enable_Depth_Test)
    _dsv_image.destroy();

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
    frame.swapchain_image.resize(_swapchain.Get(), i);
    frame.image.resize(width, height);
  }
  if (Enable_Depth_Test)
    _dsv_image.resize(width, height);
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

  // set render target image clear render target images
  clear_image();

  // set viewport
  auto viewport = CD3DX12_VIEWPORT{ 0.f, 0.f, static_cast<float>(frame.image.width()), static_cast<float>(frame.image.height()) };
  cmd->RSSetViewports(1, &viewport);
}

void RenderResource::render_end() noexcept
{
	auto& frame           = current_frame();
  auto& swapchain_image = _frames[_swapchain->GetCurrentBackBufferIndex()].swapchain_image;
  auto  cmd             = g_graphics_engine.cmd();

  // copy offscreen image to swapchain backbuffer
	copy(cmd, frame.image, swapchain_image);

  // set to present state
  swapchain_image.set_state(cmd, ImageState::present);

	// submit graphics commands to graphics engine
	frame.fence_value = g_graphics_engine.submit();

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
    err_if(true, "failed to present : {}", static_cast<uint32_t>(res));
  }
}

void RenderResource::clear_image() noexcept
{
  auto& frame               = current_frame();
  auto& render_target_image = frame.image;
  auto  cmd                 = g_graphics_engine.cmd();
  auto  rtv_handle          = render_target_image.rtv().cpu_handle();
  auto  dsv_handle          = D3D12_CPU_DESCRIPTOR_HANDLE{};
  if (Enable_Depth_Test)
    dsv_handle = _dsv_image.dsv().cpu_handle();

  // set render target view
  if (Enable_Depth_Test)
    cmd->OMSetRenderTargets(1, &rtv_handle, false, &dsv_handle);
  else
    cmd->OMSetRenderTargets(1, &rtv_handle, false, nullptr);

  // clear color
  render_target_image.clear_render_target(cmd);
  if (Enable_Depth_Test)
  {
    cmd->ClearDepthStencilView(dsv_handle, D3D12_CLEAR_FLAG_DEPTH, 1.f, 0, 0, nullptr);
    // set depth range
    cmd->OMSetDepthBounds(0.f, 1.f);
  }
}

void FrameBuffer::init() noexcept
{
  _vertices_indices_buffer.init(Vertices_Indices_Buffer_Size, false);
}

void FrameBuffer::upload(ID3D12GraphicsCommandList1* cmd, ui::FrameData const* data) noexcept
{
  auto vertices_offset = _vertices_indices_buffer.append_range(data->vertices());
  auto indices_offset  = _vertices_indices_buffer.append_range(data->indices());

  // get current buffer gpu address
  auto address = _vertices_indices_buffer.gpu_address();

  // set vertex buffer view
  D3D12_VERTEX_BUFFER_VIEW vertex_buffer_view{};
  vertex_buffer_view.BufferLocation = address;
  vertex_buffer_view.StrideInBytes  = sizeof(Vertex);
  vertex_buffer_view.SizeInBytes    = vertices_offset;
  cmd->IASetVertexBuffers(0, 1, &vertex_buffer_view);

  // add vertices offset
  address += vertices_offset;

  // set index buffer view
  D3D12_INDEX_BUFFER_VIEW index_buffer_view{};
  index_buffer_view.BufferLocation = address;
  index_buffer_view.SizeInBytes    = indices_offset;
  index_buffer_view.Format         = DXGI_FORMAT_R16_UINT;
  cmd->IASetIndexBuffer(&index_buffer_view);
}

}
