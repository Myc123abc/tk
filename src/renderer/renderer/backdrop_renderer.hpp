#pragma once

#include "../../util/singleton.hpp"

#include <wrl/client.h>
#include <DispatcherQueue.h>
#include <wincodec.h>
#include <windows.ui.composition.h>
#include <windows.ui.composition.desktop.h>
#include <windows.ui.composition.interop.h>
#include <windows.ui.composition.effects.h>
#include <windows.ui.composition.interactions.h>
#include <d2d1.h>

#include <unordered_map>

namespace tk::renderer {

Singleton(BackdropRenderer, g_backdrop_renderer,
public:
  void init() noexcept;
  void destroy() const noexcept;

  void create_desktop_window_target(HWND handle) noexcept;
  void destroy_desktop_window_target(HWND handle) noexcept;

private:
  auto CreateEffectBrush(ABI::Windows::Graphics::Effects::IGraphicsEffect* graphicsEffect) noexcept
  -> Microsoft::WRL::ComPtr<ABI::Windows::UI::Composition::ICompositionEffectBrush>;
  auto CreateNoiceBrush() noexcept
  -> Microsoft::WRL::ComPtr<ABI::Windows::UI::Composition::ICompositionSurfaceBrush>;
  auto CreateAcrylicBrush(D2D1_COLOR_F tintColor = {0.125f, 0.125f, 0.125f, 0.4f}, D2D1_COLOR_F luminosityColor = {0.125f, 0.125f, 0.125f, 0.8f}) noexcept
  -> Microsoft::WRL::ComPtr<ABI::Windows::UI::Composition::ICompositionBrush>;

private:
  ABI::Windows::System::IDispatcherQueueController*          _controller;
  ABI::Windows::UI::Composition::ICompositor*                _compositor;
  ABI::Windows::UI::Composition::ICompositionGraphicsDevice* _composition_device;
  IWICImagingFactory2*                                       _wic_factory;

  std::unordered_map<HWND, Microsoft::WRL::ComPtr<ABI::Windows::UI::Composition::Desktop::IDesktopWindowTarget>> _targets;
)

}
