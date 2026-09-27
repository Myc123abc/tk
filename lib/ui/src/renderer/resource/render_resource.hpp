#pragma once

#include "image.hpp"
#include "buffer.hpp"
#include "../config.hpp"
#include "../../ui/frame_data/frame_data.hpp"

#include <dcomp.h>

#include <array>

namespace tk::renderer {

class Command;

class FrameBuffer
{
public:
  void init() noexcept;

  void destroy() noexcept
  {
    g_buf_pool.destroy(_vertices_indices_buffer);
  }

  auto clear() noexcept -> FrameBuffer&
  {
    g_buf_pool[_vertices_indices_buffer].clear();
    return *this;
  }

  void upload(Command const* cmd, ui::FrameData const* data) noexcept;

  auto vertice_indices_buf_handle() const noexcept { return _vertices_indices_buffer; }

private:
  BufferHandle _vertices_indices_buffer;
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

  static constexpr auto Render_Target_Format = ImageFormat::rgba8_unorm;

  void init(HWND handle, uint width, uint height) noexcept;
  void destroy() noexcept;

  void resize(uint width, uint height) noexcept;

  void wait_frame_complete() noexcept;

  void render_begin() noexcept;
  void render_end() noexcept;

  void present(bool vsync) const noexcept;

  auto& current_frame() noexcept { return _frames[_frame_index]; }
  auto  render_target() noexcept { return current_frame().image; }
  auto  depth_stencil() noexcept { return _dsv_image;            }

private:
  struct Frame
  {
    ImageHandle                                    image;
    ImageHandle                                    swapchain_image;
    FrameBuffer                                    buffer;
    uint64                                         fence_value{};
    Microsoft::WRL::ComPtr<ID3D12CommandAllocator> cmd_alloc;
  };

  std::array<Frame, Frame_Count>              _frames;
  ImageHandle                                 _dsv_image;

  Microsoft::WRL::ComPtr<IDXGISwapChain4>     _swapchain;
  HANDLE                                      _swapchain_waitable_obj;
  Microsoft::WRL::ComPtr<IDCompositionTarget> _comp_target;
  Microsoft::WRL::ComPtr<IDCompositionVisual> _comp_visual;

  uint                                        _frame_index{};
};

}
