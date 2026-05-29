#pragma once

#include "descriptor_heap_manager.hpp"
#include "util/flag.hpp"
#include "util/base.hpp"
#include "../../util/align.hpp"
#include "util/rect.hpp"
#include "buffer.hpp"

#include <dxgi1_6.h>
#include <directx/d3dx12.h>

#include <optional>

namespace tk::renderer {

////////////////////////////////////////////////////////////////////////////////
///                             Structure
////////////////////////////////////////////////////////////////////////////////

enum class ImageType
{
  none = 0,
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

class Image
{
public:
  Image()                            = default;
  ~Image()                           = default;
  Image(Image const&)                = delete;
  Image(Image&&) noexcept            = delete;
  Image& operator=(Image const&)     = delete;
  Image& operator=(Image&&) noexcept = delete;

  void init(uint width , uint height, ImageFormat format, Flag<ImageType> type, bool use_mipmap = false) noexcept;
  void init(IDXGISwapChain1* swapchain, uint index) noexcept;
  // void init(ImageType type, HANDLE handle, uint width, uint height) noexcept;
  void init(float width, float height, Image const& src) noexcept { init(width, height, static_cast<ImageFormat>(src._format), src._type); }

  void destroy() noexcept;

  void set_state(ID3D12GraphicsCommandList1* cmd, ImageState state) noexcept;

  void resize(uint width, uint height) noexcept { if (!_handle.Get() || _width != width || _height != height) init(width, height, static_cast<ImageFormat>(_format), _type); }
  void resize(IDXGISwapChain1* swapchain, uint index) noexcept { init(swapchain, index); }

  void clear(ID3D12GraphicsCommandList1* cmd, D3D12_CPU_DESCRIPTOR_HANDLE cpu_handle, D3D12_GPU_DESCRIPTOR_HANDLE gpu_handle) const noexcept;
  void clear_render_target(ID3D12GraphicsCommandList1* cmd, std::optional<Rect> rect = {}) noexcept;
  void clear_depth_stencil(ID3D12GraphicsCommandList1* cmd, std::optional<Rect> rect = {}) noexcept;

  auto handle() const noexcept { return _handle.Get();                     }
  auto format() const noexcept { return static_cast<ImageFormat>(_format); }
  auto width()  const noexcept { return _width;                            }
  auto height() const noexcept { return _height;                           }
  auto extent() const noexcept { return uint2{ _width, _height };          }
  auto rect()   const noexcept { return Rect{ 0, 0, extent() };            }

  auto per_pixel_size() const noexcept -> uint;

  auto readback(ID3D12GraphicsCommandList1* cmd, RECT rect) noexcept -> std::pair<Microsoft::WRL::ComPtr<ID3D12Resource>, Bitmap>;

  auto srv() const noexcept { return _desc.srv; }
  auto uav() const noexcept { return _desc.uav; }
  auto rtv() const noexcept { return _desc.rtv; }
  auto dsv() const noexcept { return _desc.dsv; }
  // auto& mipmap_uavs() const noexcept { return _mipmap_uavs; }

  // void release_mipmap_uavs() noexcept;

private:
  void create_descriptor(bool use_mipmap = false) noexcept;

private:
  Microsoft::WRL::ComPtr<ID3D12Resource> _handle;
  uint                                   _width{};
  uint                                   _height{};
  DXGI_FORMAT                            _format{};
  Flag<ImageType>                        _type{};
  D3D12_RESOURCE_STATES                  _state{};

  struct Descriptors
  {
    DescriptorHandle srv{};
    DescriptorHandle uav{};
    DescriptorHandle rtv{};
    DescriptorHandle dsv{};
  };
  Descriptors _desc;

  // std::vector<DescriptorHandle>          _mipmap_uavs;
};

////////////////////////////////////////////////////////////////////////////////
///                             Copy Operations
////////////////////////////////////////////////////////////////////////////////

void copy(
  ID3D12GraphicsCommandList1* cmd,
  Image&                      src,
  LONG                        left,
  LONG                        top,
  LONG                        right,
  LONG                        bottom,
  Image&                      dst,
  uint                        x = 0,
  uint                        y = 0) noexcept;

inline void copy(ID3D12GraphicsCommandList1* cmd, Image& src, Image& dst) noexcept
{
  copy(cmd, src, 0, 0, src.width(), src.height(), dst);
}

inline void copy(
  ID3D12GraphicsCommandList1* cmd,
  Image&                      image,
  Buffer&                     upload_heap,
  uint                        offset,
  D3D12_SUBRESOURCE_DATA&     data
) noexcept
{
  image.set_state(cmd, cmd->GetType() == D3D12_COMMAND_LIST_TYPE_COPY ? ImageState::common : ImageState::copy_dst);
  UpdateSubresources(cmd, image.handle(), upload_heap.handle(), offset, 0, 1, &data);
}

inline void copy(
  ID3D12GraphicsCommandList1* cmd,
  Buffer&                     src,
  Image&                      dst,
  uint                        src_offset,
  BitmapView const&           bitmap,
  uint2                       pos
) noexcept
{
  dst.set_state(cmd, cmd->GetType() == D3D12_COMMAND_LIST_TYPE_COPY ? ImageState::common : ImageState::copy_dst);

  auto footprint = D3D12_PLACED_SUBRESOURCE_FOOTPRINT{};
  footprint.Offset             = src_offset;
  footprint.Footprint.Width    = bitmap.width;
  footprint.Footprint.Height   = bitmap.height;
  footprint.Footprint.Depth    = 1;
  footprint.Footprint.RowPitch = align(bitmap.row_pitch, D3D12_TEXTURE_DATA_PITCH_ALIGNMENT);
  footprint.Footprint.Format   = static_cast<DXGI_FORMAT>(dst.format());

  auto src_loc = CD3DX12_TEXTURE_COPY_LOCATION{ src.handle(), footprint };
  auto dst_loc = CD3DX12_TEXTURE_COPY_LOCATION{ dst.handle() };

  cmd->CopyTextureRegion(&dst_loc, pos.x, pos.y, 0, &src_loc, nullptr);
}

void copy(
  ID3D12GraphicsCommandList1* cmd,
  Image&                      src,
  LONG                        left,
  LONG                        top,
  LONG                        right,
  LONG                        bottom,
  ID3D12Resource*             readback_buffer) noexcept;

void copy(Bitmap const& src, Bitmap const& dst) noexcept;

}
