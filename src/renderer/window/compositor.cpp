#include "compositor.hpp"
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

}

namespace tk::renderer {

void Compositor::init() noexcept
{
  // initialize COM
  winrt::init_apartment(winrt::apartment_type::single_threaded);

  // create winrt dispacher
  using namespace ABI::Windows::System;
  err_if(CreateDispatcherQueueController({ sizeof(DispatcherQueueOptions), DQTYPE_THREAD_CURRENT, DQTAT_COM_ASTA },
    reinterpret_cast<IDispatcherQueueController**>(winrt::put_abi(_queue))),
    "failed to create dispatcher queue controller");

  // create compositor
  _compositor = winrt::Windows::UI::Composition::Compositor{};
}

auto Compositor::create_resource(HWND handle) const noexcept -> Resource
{
  auto res = Resource{};

  // create desktop window target
  res.target = CreateDesktopWindowTarget(_compositor, handle);

  // create root visual
  res.root = _compositor.CreateContainerVisual();
  res.root.RelativeSizeAdjustment({ 1.f, 1.f });
  res.target.Root(res.root);

  // create blur visual
  res.blur_visual = _compositor.CreateSpriteVisual();
  res.blur_visual.RelativeSizeAdjustment({ 1.f, 1.f });
  res.blur_visual.Brush(_compositor.CreateColorBrush(winrt::Windows::UI::Colors::Red()));
  res.root.Children().InsertAtBottom(res.blur_visual);

  return res;
}

}
