#include "backdrop_renderer.hpp"
#include "util/error_handling.hpp"
#include "../core.hpp"
#include "backdrop_effect/Effects.h"

#include <wrl/wrappers/corewrappers.h>

#include <cassert>

using namespace Microsoft::WRL;

namespace tk::renderer {

void BackdropRenderer::init() noexcept
{
  err_if(Windows::Foundation::Initialize(RO_INIT_SINGLETHREADED), "failed to initialize winrt");
  err_if(CreateDispatcherQueueController({ sizeof(DispatcherQueueOptions), DQTYPE_THREAD_CURRENT, DQTAT_COM_STA }, &_controller),
    "failed to create dispatcher queue controller");
  err_if(ABI::Windows::Foundation::ActivateInstance(
    Microsoft::WRL::Wrappers::HStringReference(RuntimeClass_Windows_UI_Composition_Compositor).Get(), &_compositor),
    "failed to activate instance");

  auto interop_compositor = TryAs<ABI::Windows::UI::Composition::ICompositorInterop>(_compositor);
  err_if(!interop_compositor, "failed to get interop compositor");
  err_if(interop_compositor->CreateGraphicsDevice(g_core.d2d_device(), &_composition_device), "failed to create graphics device");

  err_if(CoCreateInstance(CLSID_WICImagingFactory2, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&_wic_factory)), "failed to create wic factory");
}

void BackdropRenderer::destroy() const noexcept
{
  assert(_targets.empty());
  _wic_factory->Release();
  _composition_device->Release();
  _compositor->Release();
  _controller->Release();
  Windows::Foundation::Uninitialize();
}

void BackdropRenderer::create_desktop_window_target(HWND handle) noexcept
{
  assert(!_targets.contains(handle));
  
  _targets[handle];
  
  err_if(TryAs<ABI::Windows::UI::Composition::Desktop::ICompositorDesktopInterop>(_compositor)->CreateDesktopWindowTarget(handle, true, &_targets.at(handle)),
    "failed to create desktop window target");

  auto visual = ComPtr<ABI::Windows::UI::Composition::ISpriteVisual>{};
  err_if(_compositor->CreateSpriteVisual(&visual), "failed to create sprite visual");
  err_if(TryAs<ABI::Windows::UI::Composition::IVisual2>(visual)->put_RelativeSizeAdjustment({ 1.f, 1.f }), "failed to relative size for sprite visual");

  // use acrylic
  err_if(visual->put_Brush(CreateAcrylicBrush().Get()), "failed to push acrylic brush");

  err_if(TryAs<ICompositionTarget>(_targets.at(handle))->put_Root(TryAs<IVisual>(visual).Get()), "failed to set root visual");
}

void BackdropRenderer::destroy_desktop_window_target(HWND handle) noexcept
{
  assert(_targets.contains(handle));
  _targets.erase(handle);
}

////////////////////////////////////////////////////////////////////////////////
///                             Effect
////////////////////////////////////////////////////////////////////////////////

auto BackdropRenderer::CreateEffectBrush(ABI::Windows::Graphics::Effects::IGraphicsEffect* graphicsEffect) noexcept
-> Microsoft::WRL::ComPtr<ABI::Windows::UI::Composition::ICompositionEffectBrush>
{
	ComPtr<ICompositionEffectBrush> effectBrush{nullptr};
	ComPtr<ICompositionEffectFactory> effectFactory{nullptr};
	ThrowIfFailed(
		_compositor->CreateEffectFactory(graphicsEffect, &effectFactory)
	);
	ThrowIfFailed(
		effectFactory->CreateBrush(&effectBrush)
	);
	return effectBrush;
}

auto BackdropRenderer::CreateNoiceBrush() noexcept -> Microsoft::WRL::ComPtr<ABI::Windows::UI::Composition::ICompositionSurfaceBrush>
{
	// create drawing surface
	ComPtr<ICompositionDrawingSurface> compositionSurface{nullptr};
	ThrowIfFailed(
		_composition_device->CreateDrawingSurface(
			{256, 256},
			DirectXPixelFormat::DirectXPixelFormat_R16G16B16A16Float,
			DirectXAlphaMode::DirectXAlphaMode_Premultiplied,
			&compositionSurface
		)
	);

	// create surface brush
	ComPtr<ICompositionSurfaceBrush> surfaceBrush{nullptr};
	ThrowIfFailed(
		_compositor->CreateSurfaceBrushWithSurface(TryAs<ICompositionSurface>(compositionSurface).Get(), &surfaceBrush)
	);

	// get surface interop interface
	auto m_surfaceInterop{TryAs<ICompositionDrawingSurfaceInterop>(compositionSurface)};

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
		m_surfaceInterop->BeginDraw(nullptr, IID_PPV_ARGS(&d2dContext), &offset)
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
		m_surfaceInterop->EndDraw()
	);

	ThrowIfFailed(
		surfaceBrush->put_HorizontalAlignmentRatio(0.f)
	);
	ThrowIfFailed(
		surfaceBrush->put_VerticalAlignmentRatio(0.f)
	);

	return surfaceBrush;
}

// the following content was copied from microsoft-ui-xaml-main\microsoft-ui-xaml-main\dev\Materials\Acrylic\AcrylicBrush.cpp
// ******************************About the Luminosity-based Acrylic Recipe *****************************
//
// In 19H1, the Acryic recipe was altered to improve integration of Acrylic surfaces with Shadows using
// a Luminosity effect. Without this a ThemeShadow cast by an Acrylic surface was visible through it
// as a dark blur, resulting in a muddied appearence that did not match the Acrylic exppectations.
// A Luminosity effect is now used to reduce contrast in the Acrylic source and minimize the
// shadow's contribution to Acrylic output. See comment on Luminosity in recipe description below for details.
//
// Since ThemeShadow is only present in 19H1+ OS, the new recipe is only needed when when tarGeting this.
// In addition, RS2 did not have the needed Luminosity blend mode, so the legacy acrylic path needed to be
// maintained. For consistency, keep the legacy recipe afor all MUX downlevel configs (i.e. tarGeting RS5 and lower).
//
// *************************** Shadow-friendly Luminosity-based Recipe (19H1+) ***************************
//
//      <CompositeEffect>           <!-- Provides noise for acrylic -->
//          <OpacityEffect>     <!-- Noise texture with wrap and alpha -->
//              <BorderEffect>
//                  <Noise texture in a brush />
//              </BorderEffect>
//          </OpacityEffect>
//          <BlendEffect (Color)>       <!-- Tint -->
//              <ColorSourceEffect />   <!-- Tint color -->
//              <BlendEffect (Luminosity)>
//                  <ColorSourceEffect />   <!-- Luminosity color -->
//                  <Blur>
//                      <Backdrop in a brush/>
//                  </Blur>
//              </BlendEffect (Luminosity)>
//          </BlendEffect (Color)>
//      </CompositeEffect>
// ********************************** Legacy Recipe (MUX / RS5 and Lower ) **********************************
//
//      <BlendEffect>           <!-- Provides noise for acrylic -->
//          <OpacityEffect>     <!-- Noise texture with wrap and alpha -->
//              <BorderEffect>
//                  <Noise texture in a brush />
//              </BorderEffect>
//          </OpacityEffect>
//          <CompositeStepEffect>       <!-- Tint -->
//              <ColorSourceEffect />   <!-- Tint color -->
//              <BlendEffect>               <!-- Exclusion -->
//                  <ColorSourceEffect />   <!-- Exclusion color -->
//                  <Saturation>
//                      <Blur>
//                          <Backdrop in a brush/>
//                      </Blur>
//                  </Saturation>
//              </BlendEffect>
//          </CompositeStepEffect>
//
//      </BlendEffect>
ComPtr<ICompositionBrush> BackdropRenderer::CreateAcrylicBrush(D2D1_COLOR_F tintColor, D2D1_COLOR_F luminosityColor) noexcept
{
	// border effect
	auto borderEffect{Make<BorderEffect>()};
	borderEffect->SetExtendX(D2D1_BORDER_EDGE_MODE_WRAP);
	borderEffect->SetExtendY(D2D1_BORDER_EDGE_MODE_WRAP);
	borderEffect->SetInput(CompositionEffectSource(HStringReference(L"Noice").Get()));
	
	// opacity effect
	auto opacityEffect{Make<OpacityEffect>()};
	opacityEffect->put_Name(HStringReference(L"NoiceOpacity").Get());
	opacityEffect->SetOpacity(0.02f);
	opacityEffect->SetInput(borderEffect.Get());

	// gaussian blur
	auto blurEffect{Make<GaussianBlurEffect>()};
	blurEffect->put_Name(HStringReference(L"Blur").Get());
	blurEffect->SetBlurAmount(30.f);
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

	ComPtr<ICompositionEffectBrush> effectBrush{CreateEffectBrush(noiceBlendEffect.Get())};
	ComPtr<ICompositionBackdropBrush> backdropBrush{nullptr};
	ComPtr<ICompositionSurfaceBrush> surfaceBrush{CreateNoiceBrush()};
	ThrowIfFailed(
		TryAs<ICompositor2>(_compositor)->CreateBackdropBrush(&backdropBrush)
	);
	ThrowIfFailed(
		effectBrush->SetSourceParameter(HStringReference(L"Noice").Get(), TryAs<ICompositionBrush>(surfaceBrush).Get())
	); 
	ThrowIfFailed(
		effectBrush->SetSourceParameter(HStringReference(L"Backdrop").Get(), TryAs<ICompositionBrush>(backdropBrush).Get())
	);

	return TryAs<ICompositionBrush>(effectBrush);
}


}
