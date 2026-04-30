#pragma once

#include "image.hpp"
#include "buffer.hpp"
#include "../config.hpp"
#include "../../ui/frame_data.hpp"

#include <dcomp.h>

#include <array>

namespace tk::renderer {

class FrameBuffer
{
public:
  void init() noexcept;

  void destroy() noexcept
  {
    _vertices_indices_buffer.destroy();
  }

  auto clear() noexcept -> FrameBuffer&
  {
    _vertices_indices_buffer.clear();
    return *this;
  }

  void upload(ID3D12GraphicsCommandList1* cmd, ui::FrameData const* data) noexcept;

private:
  Buffer _vertices_indices_buffer;
};

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

  void resize(uint32_t width, uint32_t height) noexcept;

  void wait_frame_complete() noexcept;

  void render_begin() noexcept;
  void render_end() noexcept;

  void present(bool vsync) const noexcept;

  void clear_image() noexcept;
  void clear_depth_stencil() noexcept;

  auto& current_frame() noexcept { return _frames[_frame_index]; }

private:
  struct Frame
  {
    Image                                          image;
    Image                                          swapchain_image;
    FrameBuffer                                    buffer;
    uint64_t                                       fence_value{};
    Microsoft::WRL::ComPtr<ID3D12CommandAllocator> cmd_alloc;
  };

  std::array<Frame, Frame_Count>              _frames;
  Image                                       _dsv_image;

  Microsoft::WRL::ComPtr<IDXGISwapChain4>     _swapchain;
  HANDLE                                      _swapchain_waitable_obj;
  Microsoft::WRL::ComPtr<IDCompositionTarget> _comp_target;
  Microsoft::WRL::ComPtr<IDCompositionVisual> _comp_visual;

  uint32_t                                    _frame_index{};
};

}
