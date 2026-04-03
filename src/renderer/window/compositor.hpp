#pragma once

#include "../../util/singleton.hpp"

#include <Unknwn.h>
#include <winrt/windows.ui.composition.desktop.h>
#include <windows.ui.composition.interop.h>
#include <DispatcherQueue.h>

namespace tk::renderer {

Singleton(Compositor, g_compositor,
public:
  void init() noexcept;

  struct Resource
  {
    winrt::Windows::UI::Composition::Desktop::DesktopWindowTarget target{ nullptr };
    winrt::Windows::UI::Composition::ContainerVisual              root{ nullptr };
    winrt::Windows::UI::Composition::SpriteVisual                 blur_visual{ nullptr };
  };

  auto create_resource(HWND handle) const noexcept -> Resource;

private:
  winrt::Windows::System::DispatcherQueueController          _queue{ nullptr };
  winrt::Windows::UI::Composition::Compositor                _compositor{ nullptr };
  winrt::Windows::UI::Composition::CompositionGraphicsDevice _device{ nullptr };
)

}
