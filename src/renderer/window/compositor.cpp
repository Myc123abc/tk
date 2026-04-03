#include "compositor.hpp"
#include "util/error_handling.hpp"
#include "effect/effect.h"

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

winrt::Windows::UI::Composition::CompositionBrush CreateSimpleBackdropBlurBrush(
  winrt::Windows::UI::Composition::Compositor const& compositor,
  float blurAmount = 30.0f);

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
  res.blur_visual.Brush(CreateSimpleBackdropBlurBrush(_compositor));
  // res.blur_visual.Brush(_compositor.CreateColorBrush(winrt::Windows::UI::Colors::Red()));
  res.root.Children().InsertAtBottom(res.blur_visual);

  return res;
}

ComPtr<ICompositionEffectBrush> CreateEffectBrush(
  winrt::Windows::UI::Composition::Compositor const& compositor,
  ABI::Windows::Graphics::Effects::IGraphicsEffect* graphicsEffect)
{
	ComPtr<ICompositionEffectBrush> effectBrush{nullptr};
  auto projectedEffect = winrt::Windows::Graphics::Effects::IGraphicsEffect{ nullptr };
  winrt::copy_from_abi(projectedEffect, graphicsEffect);
  auto effectFactory = compositor.CreateEffectFactory(projectedEffect).as<ICompositionEffectFactory>();
	err_if(effectFactory->CreateBrush(&effectBrush), "failed to create effect brush");
	return effectBrush;
}

winrt::Windows::UI::Composition::CompositionBrush CreateSimpleBackdropBlurBrush(
  winrt::Windows::UI::Composition::Compositor const& compositor,
  float blurAmount)
{
  auto backdropFallbackEffect{Make<CompositeStepEffect>()};
	backdropFallbackEffect->SetCompositeMode(D2D1_COMPOSITE_MODE_SOURCE_OVER);
	backdropFallbackEffect->SetDestination(CompositionEffectSource(HStringReference(L"WallpaperBackdrop").Get()));
	backdropFallbackEffect->SetSource(CompositionEffectSource(HStringReference(L"Backdrop").Get()));

	auto blurEffect{ Make<GaussianBlurEffect>() };
	blurEffect->put_Name(HStringReference(L"Blur").Get());
	blurEffect->SetBlurAmount(blurAmount);
	blurEffect->SetBorderMode(D2D1_BORDER_MODE_HARD);
  blurEffect->SetInput(backdropFallbackEffect.Get());

	ComPtr<ICompositionEffectBrush> effectBrush{ CreateEffectBrush(compositor, blurEffect.Get()) };

  auto backdropBrush = compositor.CreateBackdropBrush();
	ThrowIfFailed(
		effectBrush->SetSourceParameter(
			HStringReference(L"Backdrop").Get(),
      backdropBrush.as<ICompositionBrush>().get()
		)
	);

	ThrowIfFailed(
		effectBrush->SetSourceParameter(
			HStringReference(L"WallpaperBackdrop").Get(),
			backdropBrush.as<ICompositionBrush>().get()
		)
	);

  auto projectedBrush = winrt::Windows::UI::Composition::CompositionBrush{ nullptr };
  winrt::copy_from_abi(projectedBrush, effectBrush.Get());
  return projectedBrush;
}

}
