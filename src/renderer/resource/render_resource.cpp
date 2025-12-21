#include "render_resource.hpp"
#include "../core.hpp"
#include "../engine/graphics_engine.hpp"

using namespace Microsoft::WRL;

namespace tk { namespace renderer {

void RenderResource::init(HWND handle, uint32_t width, uint32_t height) noexcept
{
  // create offscreen images
  for (auto [i, frame] : _frames | std::views::enumerate)
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

  // disable alt-enter fullscreen
  err_if(g_core.factory()->MakeWindowAssociation(handle, DXGI_MWA_NO_ALT_ENTER | DXGI_MWA_NO_WINDOW_CHANGES), "failed to disable alt-enter");

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
    err_if(g_core.device()->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&frame.cmd_alloc)),
            "failed to create command allocator");
  err_if(g_core.device()->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, _frames[0].cmd_alloc.Get(), nullptr, IID_PPV_ARGS(&_cmd)),
          "failed to create command list");
  err_if(_cmd->Close(), "failed to close command list");
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
  }
}

auto RenderResource::has_free_frame() const noexcept -> bool
{
  // TODO: currently, only wait graphics engine finish?
  return g_graphics_engine.fence_completed_value() >= _frames[_frame_index].fence_value;
}

void RenderResource::render_begin() const noexcept
{
  err_if(_frames[_frame_index].cmd_alloc->Reset() == E_FAIL, "failed to reset command allocator");
  err_if(_cmd->Reset(_frames[_frame_index].cmd_alloc.Get(), nullptr), "failed to reset command list");

  g_desc_heap_mgr.bind_heaps(_cmd.Get());
}

void RenderResource::render_end() noexcept
{ 
  g_image_pool[_frames[_frame_index].swapchain_image].set_state(_cmd.Get(), ImageState::present);

  g_graphics_engine.submit({ _cmd.Get() });

  _frame_index = (_frame_index + 1) % Frame_Count;
}

}}
