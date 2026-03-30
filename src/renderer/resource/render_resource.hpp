#pragma once

#include "image.hpp"
#include "buffer.hpp"
#include "../config.hpp"
#include "../compositor.hpp"

#include <dcomp.h>

#include <array>

namespace tk::renderer {

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

  void wait_frame_complete() const noexcept;

  void render_begin() noexcept;
  void render_end() noexcept;

  void present(bool vsync) const noexcept;

  void clear_image() noexcept;

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

  HWND                                        _handle{};

  std::array<Frame, Frame_Count>              _frames;
  Image                                       _dsv_image;

  Microsoft::WRL::ComPtr<IDXGISwapChain4>     _swapchain;
  HANDLE                                      _swapchain_waitable_obj;
  Microsoft::WRL::ComPtr<IDCompositionTarget> _comp_target;
  Microsoft::WRL::ComPtr<IDCompositionVisual> _comp_visual;

  Compositor::Resource                        _comp_res;

  uint32_t                                    _frame_index{};

  enum class BackdropType
  {
    transparent,
    blur,
  } _backdrop_type{};
};

}
