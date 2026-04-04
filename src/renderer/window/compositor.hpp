#pragma once

#include "../../util/singleton.hpp"

#include <Unknwn.h>
#include <winrt/windows.ui.composition.desktop.h>
#include <windows.ui.composition.interop.h>
#include <DispatcherQueue.h>
#include <wrl/client.h>
#include <windows.ui.composition.h>
#include <wincodec.h>

namespace tk::renderer {

Singleton(Compositor, g_compositor,
public:
  void init() noexcept;
  void destroy() const noexcept;

  struct Resource
  {
    winrt::Windows::UI::Composition::Desktop::DesktopWindowTarget target{ nullptr };
    winrt::Windows::UI::Composition::ContainerVisual              root{ nullptr };
    winrt::Windows::UI::Composition::SpriteVisual                 blur_visual{ nullptr };

    void update(float blur_radius) noexcept;
  };

  auto create_resource(HWND handle, float blur_radius) const noexcept -> Resource;

  auto CreateNoiceBrush() -> winrt::Windows::UI::Composition::ICompositionSurfaceBrush;

private:
  winrt::Windows::System::DispatcherQueueController          _queue{ nullptr };
  winrt::Windows::UI::Composition::Compositor                _compositor{ nullptr };
  winrt::Windows::UI::Composition::CompositionGraphicsDevice _device{ nullptr };
  IWICImagingFactory2*                                       _wic_factory;
)

}
