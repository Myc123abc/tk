#pragma once

#include "descriptor_heap_manager.hpp"
#include "tk/flag.hpp"
#include "tk/base.hpp"
#include "tk/rect.hpp"
#include "resource_tack.hpp"

#include <dxgi1_6.h>
#include <directx/d3dx12.h>

#include <optional>

namespace tk::renderer {

class Command;

////////////////////////////////////////////////////////////////////////////////
///                             Structure
////////////////////////////////////////////////////////////////////////////////

enum class ImageType
{
  srv  = 1 << 0,
  uav  = 1 << 1,
  rtv  = 1 << 2,
  dsv  = 1 << 3,
};

enum class ImageFormat
{
  r8_unorm    = DXGI_FORMAT_R8_UNORM,
  bgra8_unorm = DXGI_FORMAT_B8G8R8A8_UNORM,
  rgba8_unorm = DXGI_FORMAT_R8G8B8A8_UNORM,
  d24_s8      = DXGI_FORMAT_D24_UNORM_S8_UINT,
};

enum class ImageState
{
  copy_src      = D3D12_RESOURCE_STATE_COPY_SOURCE,
  copy_dst      = D3D12_RESOURCE_STATE_COPY_DEST,
  present       = D3D12_RESOURCE_STATE_PRESENT,
  compute_rw    = D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
  common        = D3D12_RESOURCE_STATE_COMMON,
  render_target = D3D12_RESOURCE_STATE_RENDER_TARGET,
  pixel         = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
  non_pixel     = D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
  read          = D3D12_RESOURCE_STATE_GENERIC_READ,
  depth_write   = D3D12_RESOURCE_STATE_DEPTH_WRITE,
};

////////////////////////////////////////////////////////////////////////////////
///                             Bitmap
////////////////////////////////////////////////////////////////////////////////

struct BitmapView
{
  void const* data{};
  uint        width{};
  uint        height{};
  uint        row_pitch{};
};

template <typename T>
concept BitmapType = requires(T const& t)
{
  { t.to_bitmap_view() } -> std::same_as<BitmapView>;
};

struct Bitmap
{
  void* data{};
  uint  width{};
  uint  height{};
  uint  channel{};
  uint  row_pitch{};
  uint  x{};
  uint  y{};
  bool  can_free{};

  Bitmap() = default;

  template <BitmapType T>
  Bitmap(T const& bitmap) noexcept
    : data(bitmap.data), width(bitmap.width), height(bitmap.height), row_pitch(bitmap.row_pitch) {}

  Bitmap(uint width, uint height, uint channel, void* data = nullptr, bool can_free = false) noexcept
    : data(data), width(width), height(height), channel(channel), row_pitch(width * channel), can_free(can_free) {}

  void init(uint width, uint height, uint channel, void* data = nullptr, bool can_free = false) noexcept
  {
    this->data     = data;
    this->width    = width;
    this->height   = height;
    this->channel  = channel;
    row_pitch      = width * channel;
    this->can_free = can_free;
  }

  auto to_bitmap_view() const noexcept -> BitmapView
  {
    return { data, width, height, row_pitch };
  };
};

////////////////////////////////////////////////////////////////////////////////
///                               Image
////////////////////////////////////////////////////////////////////////////////

class Image : public ResourceTrack
{
public:
  Image()                        = default;
  ~Image()                       = default;
  Image(Image const&)            = delete;
  Image(Image&&)                 = delete;
  Image& operator=(Image const&) = delete;
  Image& operator=(Image&&)      = delete;

  void init(uint width , uint height, ImageFormat format, Flag<ImageType> type, bool use_mipmap = false) noexcept;
  void init(IDXGISwapChain1* swapchain, uint index) noexcept;
  // void init(ImageType type, HANDLE handle, uint width, uint height) noexcept;
  void init(float width, float height, Image const& src) noexcept { init(width, height, static_cast<ImageFormat>(src._format), src._types); }

  void destroy() noexcept;

  [[nodiscard]]
  auto transform(Command const* cmd, Flag<ImageState> states, uint subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES) noexcept -> std::vector<D3D12_RESOURCE_BARRIER>;

  void resize(uint width, uint height) noexcept;
  void resize(IDXGISwapChain1* swapchain, uint index) noexcept { init(swapchain, index); }

  void clear(Command const* cmd, D3D12_CPU_DESCRIPTOR_HANDLE cpu_handle, D3D12_GPU_DESCRIPTOR_HANDLE gpu_handle) const noexcept;
  void clear_render_target(Command const* cmd, std::optional<Rect> rect = {}) noexcept;
  void clear_depth_stencil(Command const* cmd, std::optional<Rect> rect = {}) noexcept;

  auto handle() const noexcept { return _handle.Get();                     }
  auto format() const noexcept { return static_cast<ImageFormat>(_format); }
  auto types()  const noexcept { return _types;                            }
  auto width()  const noexcept { return _width;                            }
  auto height() const noexcept { return _height;                           }
  auto extent() const noexcept { return uint2{ _width, _height };          }
  auto rect()   const noexcept { return Rect{ 0, 0, extent() };            }

  auto per_pixel_size() const noexcept -> uint;

  auto  srv()          const noexcept { return _desc.srv;     }
  auto  uav()          const noexcept { return _desc.uav;     }
  auto  rtv()          const noexcept { return _desc.rtv;     }
  auto  dsv()          const noexcept { return _desc.dsv;     }
  auto& mipmap_descs() const noexcept { return _mipmap_descs; }

  void release_mipmap_descs() noexcept;

private:
  void create_descriptor(bool use_mipmap = false) noexcept;
  void create_mipmap_descs() noexcept;

private:
  Microsoft::WRL::ComPtr<ID3D12Resource> _handle;
  uint                                   _width{};
  uint                                   _height{};
  DXGI_FORMAT                            _format{};
  Flag<ImageType>                        _types{};
  std::vector<D3D12_RESOURCE_STATES>     _states{};

  struct Descriptors
  {
    DescriptorHandle srv{};
    DescriptorHandle uav{};
    DescriptorHandle rtv{};
    DescriptorHandle dsv{};
  };
  Descriptors _desc;

  std::vector<std::pair<DescriptorHandle, DescriptorHandle>> _mipmap_descs;
};

}
