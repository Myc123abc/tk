#include "compositor.hpp"
#include "core.hpp"
#include "util/error_handling.hpp"

namespace {

inline auto CreateDesktopWindowTarget(
  winrt::Windows::UI::Composition::Compositor const& compositor, HWND window)-> winrt::Windows::UI::Composition::Desktop::DesktopWindowTarget
{
	namespace abi = ABI::Windows::UI::Composition::Desktop;

	auto interop = compositor.as<abi::ICompositorDesktopInterop>();
	auto target = winrt::Windows::UI::Composition::Desktop::DesktopWindowTarget{ nullptr };
	winrt::check_hresult(interop->CreateDesktopWindowTarget(window, true, reinterpret_cast<abi::IDesktopWindowTarget**>(winrt::put_abi(target))));
	return target;
}

inline auto CreateCompositionGraphicsDevice(
    winrt::Windows::UI::Composition::Compositor const& compositor,
    ::IUnknown* device)
{
  winrt::Windows::UI::Composition::CompositionGraphicsDevice graphicsDevice{ nullptr };
  auto compositorInterop = compositor.as<ABI::Windows::UI::Composition::ICompositorInterop>();
  winrt::com_ptr<ABI::Windows::UI::Composition::ICompositionGraphicsDevice> graphicsInterop;
  winrt::check_hresult(compositorInterop->CreateGraphicsDevice(device, graphicsInterop.put()));
  winrt::check_hresult(graphicsInterop->QueryInterface(winrt::guid_of<winrt::Windows::UI::Composition::CompositionGraphicsDevice>(),
      reinterpret_cast<void**>(winrt::put_abi(graphicsDevice))));
  return graphicsDevice;
}

inline auto CreateCompositionSurfaceForSwapChain(
    winrt::Windows::UI::Composition::Compositor const& compositor,
    ::IUnknown* swapChain)
{
  winrt::Windows::UI::Composition::ICompositionSurface surface{ nullptr };
  auto compositorInterop = compositor.as<ABI::Windows::UI::Composition::ICompositorInterop>();
  winrt::com_ptr<ABI::Windows::UI::Composition::ICompositionSurface> surfaceInterop;
  winrt::check_hresult(compositorInterop->CreateCompositionSurfaceForSwapChain(swapChain, surfaceInterop.put()));
  winrt::check_hresult(surfaceInterop->QueryInterface(winrt::guid_of<winrt::Windows::UI::Composition::ICompositionSurface>(),
      reinterpret_cast<void**>(winrt::put_abi(surface))));
  return surface;
}

inline void ResizeSurface(
    winrt::Windows::UI::Composition::CompositionDrawingSurface const& surface,
    winrt::Windows::Foundation::Size const& size)
{
  auto surfaceInterop = surface.as<ABI::Windows::UI::Composition::ICompositionDrawingSurfaceInterop>();
  SIZE newSize = {};
  newSize.cx = static_cast<LONG>(std::round(size.Width));
  newSize.cy = static_cast<LONG>(std::round(size.Height));
  winrt::check_hresult(surfaceInterop->Resize(newSize));
}

}

namespace tk::renderer {

void Compositor::init() noexcept
{
  // create winrt dispacher
  using namespace ABI::Windows::System;
  err_if(CreateDispatcherQueueController({ sizeof(DispatcherQueueOptions), DQTYPE_THREAD_CURRENT, DQTAT_COM_ASTA },
    reinterpret_cast<IDispatcherQueueController**>(winrt::put_abi(_queue))),
    "failed to create dispatcher queue controller");

  // create compositor
  _compositor = winrt::Windows::UI::Composition::Compositor{};

  // create compositor graphics device
  _device = CreateCompositionGraphicsDevice(_compositor, g_core.device_11());
}

auto Compositor::create_resource(HWND handle, IUnknown* swapchain) const noexcept -> Resource
{
  auto res = Resource{};

  // create desktop window target
  res.target = CreateDesktopWindowTarget(_compositor, handle);

  // create root visual
  res.root = _compositor.CreateContainerVisual();
  res.root.RelativeSizeAdjustment({ 1.f, 1.f });

  // assign root
  res.target.Root(res.root);

  // create surface
  auto surface = CreateCompositionSurfaceForSwapChain(_compositor, swapchain);

  // create surface brush
  auto brush = _compositor.CreateSurfaceBrush(surface);

  // create visual
  res.swapchain_visual = _compositor.CreateSpriteVisual();
  res.swapchain_visual.RelativeSizeAdjustment({ 1.f, 1.f });
  res.swapchain_visual.Brush(brush);

  // add swapchain visual
  res.root.Children().InsertAtBottom(res.swapchain_visual);

  // TODO: add blur visual

  return res;
}

}
