#include "render_resource.hpp"
#include "../core.hpp"
#include "../engine/graphics_engine.hpp"

using namespace Microsoft::WRL;

namespace tk { namespace renderer {

void RenderResource::init(HWND handle, uint32_t width, uint32_t height) noexcept
{
  // create offscreen images
  for (auto [i, img] : images | std::views::enumerate)
  {
    img = g_image_pool.alloc();
    g_image_pool[img].init(ImageType::rtv, Render_Target_Format, width, height);
  }
  
  // create depth test image
  if (Enable_Depth_Test)
  {
    dsv_image = g_image_pool.alloc();
    g_image_pool[dsv_image].init(ImageType::dsv, ImageFormat::d32, width, height);
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
  for (auto [i, img] : _swapchain_images | std::views::enumerate)
  {
    img = g_image_pool.alloc();
    g_image_pool[img].init(_swapchain.Get(), i);
  }
}

void RenderResource::destroy() noexcept
{
  CloseHandle(_swapchain_waitable_obj);
  if (Enable_Depth_Test)
    g_image_pool[dsv_image].destroy();
  for (auto i : std::views::iota(0, Frame_Count))
  {
    g_image_pool.free(images[i]);
    g_image_pool.free(_swapchain_images[i]);
  }
}

}}
