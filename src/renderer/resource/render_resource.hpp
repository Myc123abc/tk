#pragma once

#include "image.hpp"
#include "../config.hpp"

#include <dcomp.h>

#include <array>

namespace tk { namespace renderer {

struct RenderResource
{
  static constexpr auto Render_Target_Format = ImageFormat::bgra8_unorm;

  std::array<ImageHandle, Frame_Count> images;
  ImageHandle                          dsv_image;

private:
  Microsoft::WRL::ComPtr<IDXGISwapChain4>     _swapchain;
  std::array<ImageHandle, Frame_Count>        _swapchain_images;
  HANDLE                                      _swapchain_waitable_obj;

  Microsoft::WRL::ComPtr<IDCompositionTarget> _comp_target;
  Microsoft::WRL::ComPtr<IDCompositionVisual> _comp_visual;

public:
  void init(HWND handle, uint32_t width, uint32_t height) noexcept;
  void destroy() noexcept;
};


}}
