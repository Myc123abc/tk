#pragma once

#include "image.hpp"
#include "../config.hpp"

#include <dcomp.h>

#include <array>

namespace tk { namespace renderer {

class RenderResource
{
public:
  RenderResource()                                 = default;
  ~RenderResource()                                = default;
  RenderResource(RenderResource const&)            = delete;
  RenderResource(RenderResource&&)                 = default;
  RenderResource& operator=(RenderResource const&) = delete;
  RenderResource& operator=(RenderResource&&)      = delete;

  static constexpr auto Render_Target_Format = ImageFormat::bgra8_unorm;

  void init(HWND handle, uint32_t width, uint32_t height) noexcept;
  void destroy() noexcept;

  auto has_free_frame() const noexcept -> bool;
  void render_begin() const noexcept;
  void render_end() noexcept;

private:
  struct Frame
  {
    ImageHandle                                    image;
    ImageHandle                                    swapchain_image;
    uint64_t                                       fence_value{};
    Microsoft::WRL::ComPtr<ID3D12CommandAllocator> cmd_alloc;
  };

  std::array<Frame, Frame_Count>                     _frames;
  ImageHandle                                        _dsv_image;

  Microsoft::WRL::ComPtr<IDXGISwapChain4>            _swapchain;
  HANDLE                                             _swapchain_waitable_obj;
  Microsoft::WRL::ComPtr<IDCompositionTarget>        _comp_target;
  Microsoft::WRL::ComPtr<IDCompositionVisual>        _comp_visual;

  uint32_t                                           _frame_index{};
  Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList1> _cmd;

};


}}
