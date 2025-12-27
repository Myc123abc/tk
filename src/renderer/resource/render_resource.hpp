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

  void render_begin() noexcept;
  void render_end() noexcept;

  void present(bool vsync) const noexcept;

  void clear_image() noexcept;
  
  auto graphics_cmd() const noexcept { return _graphics_cmd.Get(); }

  auto& current_frame() noexcept { return _frames[_frame_index]; }

private:
  struct Frame
  {
    ImageHandle                                    image;
    ImageHandle                                    swapchain_image;
    FrameBuffer                                    buffer;

    uint64_t                                       graphics_fence_value{};
    Microsoft::WRL::ComPtr<ID3D12CommandAllocator> graphics_cmd_alloc;
    uint64_t                                       copy_fence_value{};
    Microsoft::WRL::ComPtr<ID3D12CommandAllocator> copy_cmd_alloc;
  };

  std::array<Frame, Frame_Count>                     _frames;
  ImageHandle                                        _dsv_image;

  Microsoft::WRL::ComPtr<IDXGISwapChain4>            _swapchain;
  HANDLE                                             _swapchain_waitable_obj;
  Microsoft::WRL::ComPtr<IDCompositionTarget>        _comp_target;
  Microsoft::WRL::ComPtr<IDCompositionVisual>        _comp_visual;

  uint32_t                                           _frame_index{};

  Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList1> _graphics_cmd;
  Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList1> _copy_cmd;
};


}}
