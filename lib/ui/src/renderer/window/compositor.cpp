#include "compositor.hpp"
#include "tk/error_handling.hpp"
#include "effect/effect.h"
#include "../core.hpp"

using namespace tk;
using namespace tk::renderer;

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

auto CreateEffectBrush(
  winrt::Windows::UI::Composition::Compositor const& compositor,
  ABI::Windows::Graphics::Effects::IGraphicsEffect* graphicsEffect) -> ComPtr<ICompositionEffectBrush>
{
  auto projectedEffect = winrt::Windows::Graphics::Effects::IGraphicsEffect{ nullptr };
  winrt::copy_from_abi(projectedEffect, graphicsEffect);

	auto effectBrush = ComPtr<ICompositionEffectBrush>{};
  err_if(compositor.CreateEffectFactory(projectedEffect).as<ICompositionEffectFactory>()->CreateBrush(&effectBrush),
    "failed to create effect brush");
	return effectBrush;
}

auto CreateBackdropBlurBrush(
  winrt::Windows::UI::Composition::Compositor const& compositor,
  float blurAmount)
-> winrt::Windows::UI::Composition::CompositionBrush
{
  auto backdropFallbackEffect{ Make<CompositeStepEffect>() };
	backdropFallbackEffect->SetCompositeMode(D2D1_COMPOSITE_MODE_SOURCE_OVER);
	backdropFallbackEffect->SetDestination(CompositionEffectSource(HStringReference(L"WallpaperBackdrop").Get()));
	backdropFallbackEffect->SetSource(CompositionEffectSource(HStringReference(L"Backdrop").Get()));

	auto blurEffect{ Make<GaussianBlurEffect>() };
	blurEffect->put_Name(HStringReference(L"Blur").Get());
	blurEffect->SetBlurAmount(blurAmount);
	blurEffect->SetBorderMode(D2D1_BORDER_MODE_HARD);
  blurEffect->SetInput(backdropFallbackEffect.Get());

	auto effectBrush{ CreateEffectBrush(compositor, blurEffect.Get()) };

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

auto CreateAcrylicBrush(
  winrt::Windows::UI::Composition::Compositor const& compositor,
  float opacity, float blur, float4 tint_color, float4 luminosity_color)
-> winrt::Windows::UI::Composition::CompositionBrush
{
  D2D1_COLOR_F tintColor{ tint_color.x, tint_color.y, tint_color.z, tint_color.w };
  D2D1_COLOR_F luminosityColor{ luminosity_color.x, luminosity_color.y, luminosity_color.z, luminosity_color.w };
	// border effect
	auto borderEffect{Make<BorderEffect>()};
	borderEffect->SetExtendX(D2D1_BORDER_EDGE_MODE_WRAP);
	borderEffect->SetExtendY(D2D1_BORDER_EDGE_MODE_WRAP);
	borderEffect->SetInput(CompositionEffectSource(HStringReference(L"Noice").Get()));
	
	// opacity effect
	auto opacityEffect{Make<OpacityEffect>()};
	opacityEffect->put_Name(HStringReference(L"NoiceOpacity").Get());
	opacityEffect->SetOpacity(opacity);
	opacityEffect->SetInput(borderEffect.Get());

	// gaussian blur
	auto blurEffect{Make<GaussianBlurEffect>()};
	blurEffect->put_Name(HStringReference(L"Blur").Get());
	blurEffect->SetBlurAmount(blur);
	blurEffect->SetBorderMode(D2D1_BORDER_MODE_HARD);
	blurEffect->SetInput(CompositionEffectSource(HStringReference(L"Backdrop").Get()));

	// tint Color
	auto tintColorEffect{Make<ColorSourceEffect>()};
	tintColorEffect->put_Name(HStringReference(L"TintColor").Get());
	tintColorEffect->SetColor(tintColor);

	// luminosity Color
	auto luminosityColorEffect{Make<ColorSourceEffect>()};
	luminosityColorEffect->put_Name(HStringReference(L"LuminosityColor").Get());
	luminosityColorEffect->SetColor(luminosityColor);

	// luminosity blend
	// NOTE: There is currently a bug where the names of BlendEffectMode::Luminosity and BlendEffectMode::Color are flipped.
	// This should be changed to Luminosity when/if the bug is fixed.
	auto luminosityBlendEffect{Make<BlendEffect>()};
	luminosityBlendEffect->SetBlendMode(D2D1_BLEND_MODE_COLOR);
	luminosityBlendEffect->SetBackground(blurEffect.Get());
	luminosityBlendEffect->SetForeground(luminosityColorEffect.Get());

	// color blend
	// NOTE: There is currently a bug where the names of BlendEffectMode::Luminosity and BlendEffectMode::Color are flipped.
	// This should be changed to Color when/if the bug is fixed.
	auto colorBlendEffect{Make<BlendEffect>()};
	colorBlendEffect->SetBlendMode(D2D1_BLEND_MODE_LUMINOSITY);
	colorBlendEffect->SetBackground(luminosityBlendEffect.Get());
	colorBlendEffect->SetForeground(tintColorEffect.Get());

	// noice blend
	auto noiceBlendEffect{Make<BlendEffect>()};
	noiceBlendEffect->SetBlendMode(D2D1_BLEND_MODE_MULTIPLY);
	noiceBlendEffect->SetBackground(colorBlendEffect.Get());
	noiceBlendEffect->SetForeground(opacityEffect.Get());

	ComPtr<ICompositionEffectBrush> effectBrush{CreateEffectBrush(compositor, noiceBlendEffect.Get())};
	ComPtr<ICompositionBackdropBrush> backdropBrush{nullptr};
  auto surfaceBrush = g_compositor.CreateNoiceBrush().as<ICompositionSurfaceBrush>();

	ThrowIfFailed(
    compositor.as<ICompositor2>()->CreateBackdropBrush(&backdropBrush)
	);
	ThrowIfFailed(
		effectBrush->SetSourceParameter(HStringReference(L"Noice").Get(), surfaceBrush.as<ICompositionBrush>().get())
	); 
	ThrowIfFailed(
		effectBrush->SetSourceParameter(HStringReference(L"Backdrop").Get(), TryAs<ICompositionBrush>(backdropBrush).Get())
	);

  auto projectedBrush = winrt::Windows::UI::Composition::CompositionBrush{ nullptr };
  winrt::copy_from_abi(projectedBrush, effectBrush.Get());
  return projectedBrush;
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

  // create compositor graphics device
  _device = CreateCompositionGraphicsDevice(_compositor, g_core.device_d2d());

  // create wic factory
	err_if(CoCreateInstance(CLSID_WICImagingFactory2, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&_wic_factory)),
    "failed to create wic factory");
}

void Compositor::destroy() const noexcept
{
  _wic_factory->Release();
}

auto Compositor::create_resource(HWND handle, ui::Backdrop const& backdrop) const noexcept -> Resource
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

  res.backdrop = backdrop;
  res.create_blur_visual();
  res.root.Children().InsertAtBottom(res.blur_visual);

  return res;
}

void Compositor::Resource::create_blur_visual() noexcept
{
  // HACK:
  // when duplicate monitors, this blur backdrop brush will invalid on some monitors, but original color brush is ok
  // now i don't process it, because it's a rarely usage for me
  assert(backdrop.style != ui::BackdropStyle::none);
  if (backdrop.style == ui::BackdropStyle::blur)
    blur_visual.Brush(CreateBackdropBlurBrush(g_compositor._compositor, backdrop.blur_radius));
  else if (backdrop.style == ui::BackdropStyle::acrylic)
    blur_visual.Brush(CreateAcrylicBrush(g_compositor._compositor,
      backdrop.acrylic.opacity, backdrop.acrylic.blur, backdrop.acrylic.tint_color, backdrop.acrylic.luminosity_color));
}

void Compositor::Resource::update(ui::Backdrop const& backdrop) noexcept
{
  if (this->backdrop != backdrop)
  {
    this->backdrop = backdrop;
    create_blur_visual();
  }
}

auto Compositor::CreateNoiceBrush() -> winrt::Windows::UI::Composition::ICompositionSurfaceBrush
{
	// create drawing surface
	auto compositionSurface =
		_device.CreateDrawingSurface(
			{256, 256},
			winrt::Windows::Graphics::DirectX::DirectXPixelFormat::R16G16B16A16Float,
			winrt::Windows::Graphics::DirectX::DirectXAlphaMode::Premultiplied);

	// create surface brush
	auto surfaceBrush = _compositor.CreateSurfaceBrush(compositionSurface);

	// get surface interop interface
  auto surface_interop = compositionSurface.as<ICompositionDrawingSurfaceInterop>();

	// load shared noice texture from system components
	HINSTANCE hModule = LoadLibraryEx(_T("Windows.UI.Xaml.Controls.dll"), nullptr, LOAD_LIBRARY_SEARCH_SYSTEM32 | LOAD_LIBRARY_AS_DATAFILE | LOAD_LIBRARY_AS_IMAGE_RESOURCE);
	ThrowIfFailed(hModule ? S_OK : HRESULT_FROM_WIN32(GetLastError()));

	HRSRC hResource = FindResource(hModule, MAKEINTRESOURCE(2000), RT_RCDATA);
	ThrowIfFailed(hResource ? S_OK : HRESULT_FROM_WIN32(GetLastError()));

	HGLOBAL hGlobal = LoadResource(hModule, hResource);
	ThrowIfFailed(hGlobal ? S_OK : HRESULT_FROM_WIN32(GetLastError()));

	DWORD dwResourceSize = SizeofResource(hModule, hResource);
	ThrowIfFailed(dwResourceSize ? S_OK : HRESULT_FROM_WIN32(GetLastError()));

	BYTE* pbResource = (BYTE*)LockResource(hGlobal);
	ThrowIfFailed(pbResource ? S_OK : HRESULT_FROM_WIN32(GetLastError()));

	ComPtr<IStream> stream{SHCreateMemStream(pbResource, dwResourceSize)};
	ThrowIfFailed(stream ? S_OK : HRESULT_FROM_WIN32(GetLastError()));

	UnlockResource(hGlobal);
	FreeResource(hGlobal);
	FreeLibrary(hModule);

	// create direct2d bitmap from wic
	ComPtr<IWICBitmapDecoder> wicDecorder{nullptr};
	ThrowIfFailed(_wic_factory->CreateDecoderFromStream(stream.Get(), &GUID_VendorMicrosoft, WICDecodeMetadataCacheOnDemand, &wicDecorder));

	ComPtr<IWICBitmapFrameDecode> wicFrame{nullptr};
	ThrowIfFailed(wicDecorder->GetFrame(0, &wicFrame));

	ComPtr<IWICFormatConverter> wicConverter;
	ThrowIfFailed(_wic_factory->CreateFormatConverter(&wicConverter));

	ComPtr<IWICPalette> wicPalette{nullptr};
	ThrowIfFailed(
		wicConverter->Initialize(
			wicFrame.Get(),
			GUID_WICPixelFormat32bppPBGRA,
			WICBitmapDitherTypeNone,
			wicPalette.Get(),
			0, WICBitmapPaletteTypeCustom
		)
	);

	ComPtr<IWICBitmap> wicBitmap{nullptr};
	ThrowIfFailed(_wic_factory->CreateBitmapFromSource(wicConverter.Get(), WICBitmapCreateCacheOption::WICBitmapNoCache, &wicBitmap));

	// render texture to visual surface
	ComPtr<ID2D1Bitmap1> d2dBitmap{nullptr};
	ComPtr<ID2D1DeviceContext> d2dContext{nullptr};

	POINT offset = {0, 0};
	ThrowIfFailed(
		surface_interop->BeginDraw(nullptr, IID_PPV_ARGS(&d2dContext), &offset)
	);
	d2dContext->Clear();
	d2dContext->CreateBitmapFromWicBitmap(
		wicBitmap.Get(),
		BitmapProperties1(
			D2D1_BITMAP_OPTIONS_NONE,
			PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED)
		),
		&d2dBitmap
	);
	d2dContext->DrawBitmap(d2dBitmap.Get());
	ThrowIfFailed(
		surface_interop->EndDraw()
	);

  surfaceBrush.HorizontalAlignmentRatio(0.f);
  surfaceBrush.VerticalAlignmentRatio(0.f);

  return surfaceBrush;
}

}
