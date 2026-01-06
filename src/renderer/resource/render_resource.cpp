#include "render_resource.hpp"
#include "../core.hpp"
#include "../engine/graphics_engine.hpp"

#include <directx/d3dx12.h>
#include <windows.h>

using namespace Microsoft::WRL;

namespace tk { namespace renderer {

void RenderResource::init(HWND handle, uint32_t width, uint32_t height) noexcept
{
  // create offscreen images
  for (auto& frame : _frames)
  {
    frame.image = g_image_pool.alloc();
    g_image_pool[frame.image].init(ImageType::rtv, Render_Target_Format, width, height);
  }
  
  // create depth test image
  if (Enable_Depth_Test)
  {
    _dsv_image = g_image_pool.alloc();
    g_image_pool[_dsv_image].init(ImageType::dsv, ImageFormat::d32, width, height);
  }

  // create swapchain
  ComPtr<IDXGISwapChain1> swapchain;
  DXGI_SWAP_CHAIN_DESC1 swapchain_desc{};
  swapchain_desc.BufferCount      = Frame_Count;
  swapchain_desc.Width            = width;
  swapchain_desc.Height           = height;
  swapchain_desc.Format           = dxgi_format(Render_Target_Format);
  swapchain_desc.BufferUsage      = DXGI_USAGE_RENDER_TARGET_OUTPUT;
  swapchain_desc.SwapEffect       = DXGI_SWAP_EFFECT_FLIP_DISCARD;
  swapchain_desc.SampleDesc.Count = 1;
  swapchain_desc.Flags            = DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT | DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING;
  swapchain_desc.AlphaMode        = DXGI_ALPHA_MODE_PREMULTIPLIED;
  err_if(g_core.factory()->CreateSwapChainForComposition(g_graphics_engine.queue(), &swapchain_desc, nullptr, &swapchain),
          "failed to create swapchain for composition");

  // create composition
  err_if(g_core.comp_device()->CreateTargetForHwnd(handle, true, &_comp_target),
          "failed to create composition target");
  err_if(g_core.comp_device()->CreateVisual(&_comp_visual),
          "failed to create composition visual");
  err_if(_comp_visual->SetContent(swapchain.Get()),
          "failed to bind swapchain to composition visual");
  err_if(_comp_target->SetRoot(_comp_visual.Get()),
          "failed to bind composition visual to target");
  err_if(g_core.comp_device()->Commit(),
          "failed to commit composition device");

  // set swapchain property and get waitable object
  err_if(swapchain.As(&_swapchain), "failed to get swapchain4");
  _swapchain->SetMaximumFrameLatency(Frame_Count);
  _swapchain_waitable_obj = _swapchain->GetFrameLatencyWaitableObject();
  err_if(!_swapchain_waitable_obj, "failed to get waitable object from swapchain");

  // get image from swapchain backbuffers
  for (auto [i, frame] : _frames | std::views::enumerate)
  {
    frame.swapchain_image = g_image_pool.alloc();
    g_image_pool[frame.swapchain_image].init(_swapchain.Get(), i);
  }

  // create command allocator and list
  for (auto& frame : _frames)
  {
		frame.graphics_cmd_alloc = g_core.create_cmd_alloc(D3D12_COMMAND_LIST_TYPE_DIRECT);
		frame.copy_cmd_alloc     = g_core.create_cmd_alloc(D3D12_COMMAND_LIST_TYPE_COPY);

    // initialize frame buffer
    frame.buffer.init();
  }
	_graphics_cmd = g_core.create_cmd(D3D12_COMMAND_LIST_TYPE_DIRECT, _frames[0].graphics_cmd_alloc.Get());
	_copy_cmd     = g_core.create_cmd(D3D12_COMMAND_LIST_TYPE_COPY, _frames[0].copy_cmd_alloc.Get());
}

void RenderResource::destroy() noexcept
{
  CloseHandle(_swapchain_waitable_obj);
  if (Enable_Depth_Test)
    g_image_pool[_dsv_image].destroy();
  for (auto& frame : _frames)
  {
    g_image_pool.free(frame.image);
    g_image_pool.free(frame.swapchain_image);
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
    g_image_pool[frame.swapchain_image].destroy();
    g_image_pool[frame.image].destroy();
  }
  _comp_visual->SetContent(nullptr);
  if (Enable_Depth_Test)
    g_image_pool[_dsv_image].destroy();

  // resize swapchain
  err_if(_swapchain->ResizeBuffers(Frame_Count, width, height, DXGI_FORMAT_UNKNOWN, DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT | DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING),
          "failed to resize swapchain");

  // rebind composition resources
  err_if(_comp_visual->SetContent(_swapchain.Get()),
          "failed to bind swapchain to composition visual");
  err_if(g_core.comp_device()->Commit(),
          "failed to commit composition device");
  
  // recreate images
  for (auto [i, frame] : _frames | std::views::enumerate)
  {
    g_image_pool[frame.swapchain_image].resize(_swapchain.Get(), i);
    g_image_pool[frame.image].resize(width, height);
  }
  if (Enable_Depth_Test)
    g_image_pool[_dsv_image].resize(width, height);
}

void RenderResource::wait_frame_complete() const noexcept
{
  auto& frame = _frames[_frame_index];
  if (g_graphics_engine.fence_completed_value() < frame.graphics_fence_value)
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

  // reset command list
  err_if(frame.graphics_cmd_alloc->Reset() == E_FAIL, "failed to reset command allocator");
  err_if(_graphics_cmd->Reset(frame.graphics_cmd_alloc.Get(), nullptr), "failed to reset command list");

  // bind heaps
  g_desc_heap_mgr.bind_heaps(_graphics_cmd.Get());

  // set render target image clear render target images
  clear_image();

  // set viewport
  auto& image    = g_image_pool[frame.image];
  auto  viewport = CD3DX12_VIEWPORT{ 0.f, 0.f, static_cast<float>(image.width()), static_cast<float>(image.height()) };
  _graphics_cmd->RSSetViewports(1, &viewport);
}

void RenderResource::render_end() noexcept
{
	auto& frame           = current_frame();
  auto& swapchain_image = g_image_pool[_frames[_swapchain->GetCurrentBackBufferIndex()].swapchain_image];

  // copy offscreen image to swapchain backbuffer
	copy(_graphics_cmd.Get(), g_image_pool[frame.image], swapchain_image);

  // set to present state
  swapchain_image.set_state(_graphics_cmd.Get(), ImageState::present);

	// submit graphics commands to graphics engine
	frame.graphics_fence_value = g_graphics_engine.submit({ _graphics_cmd.Get() });

  // move to next frame
  _frame_index = (_frame_index + 1) % Frame_Count;
}

void RenderResource::present(bool vsync) const noexcept
{
  vsync
    ? err_if(_swapchain->Present(1, 0), "failed to present swapchain")
    : err_if(_swapchain->Present(0, DXGI_PRESENT_ALLOW_TEARING), "failed to present swapchain");
}

void RenderResource::clear_image() noexcept
{
  auto& frame               = current_frame();
  auto& render_target_image = g_image_pool[frame.image];
  auto  rtv_handle          = render_target_image.cpu_handle();
  auto  dsv_handle          = D3D12_CPU_DESCRIPTOR_HANDLE{};
  if (Enable_Depth_Test)
    dsv_handle = g_image_pool[_dsv_image].cpu_handle();

  // set render target view
  if (Enable_Depth_Test)
    _graphics_cmd->OMSetRenderTargets(1, &rtv_handle, false, &dsv_handle);
  else
    _graphics_cmd->OMSetRenderTargets(1, &rtv_handle, false, nullptr);

  // clear color
  render_target_image.clear_render_target(_graphics_cmd.Get());
  if (Enable_Depth_Test)
  {
    _graphics_cmd->ClearDepthStencilView(dsv_handle, D3D12_CLEAR_FLAG_DEPTH, 1.f, 0, 0, nullptr);
    // set depth range
    _graphics_cmd->OMSetDepthBounds(0.f, 1.f);
  }
}

}}
