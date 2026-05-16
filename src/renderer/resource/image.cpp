#include "image.hpp"
#include "../core.hpp"
#include "util/error_handling.hpp"
#include "../../util/align.hpp"

using namespace tk;
using namespace tk::renderer;
using namespace Microsoft::WRL;

namespace {

template<auto T>
struct dx12_traits;

template<>
struct dx12_traits<ImageType::srv>
{
  static constexpr auto flag  = D3D12_RESOURCE_FLAG_NONE;
  static constexpr auto state = D3D12_RESOURCE_STATE_COMMON;
};

template<>
struct dx12_traits<ImageType::uav>
{
  static constexpr auto flag  = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
  static constexpr auto state = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
};

template<>
struct dx12_traits<ImageType::rtv>
{
  static constexpr auto flag  = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
  static constexpr auto state = D3D12_RESOURCE_STATE_RENDER_TARGET;
};

template<>
struct dx12_traits<ImageType::dsv>
{
  static constexpr auto flag  = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
  static constexpr auto state = D3D12_RESOURCE_STATE_DEPTH_WRITE;
};

constexpr auto dx12_resource_flags(Flag<ImageType> type) noexcept
{
  using enum ImageType;
  auto flags = D3D12_RESOURCE_FLAG_NONE;
  if (type.contains(uav)) flags |= dx12_traits<uav>::flag;
  if (type.contains(rtv)) flags |= dx12_traits<rtv>::flag;
  if (type.contains(dsv)) flags |= dx12_traits<dsv>::flag;
  return flags;
}

constexpr auto dx12_resource_states(Flag<ImageType> type) noexcept
{
  using enum ImageType;
  auto states = D3D12_RESOURCE_STATE_COMMON;
  if (type.contains(uav)) states |= dx12_traits<uav>::state;
  if (type.contains(rtv)) states |= dx12_traits<rtv>::state;
  if (type.contains(dsv)) states |= dx12_traits<dsv>::state;
  return states;
}

}

namespace tk::renderer {

void Image::destroy() noexcept
{
  _handle.Reset();
  _desc.srv.release();
  _desc.uav.release();
  _desc.rtv.release();
  _desc.dsv.release();
}

void Image::init(uint width , uint height, ImageFormat format, Flag<ImageType> type, bool use_mipmap) noexcept
{
  auto device = g_core.device();

  _width  = width;
  _height = height;
  _format = static_cast<DXGI_FORMAT>(format);
  _type   = type;
  _state  = dx12_resource_states(type);

  // create image
  auto texture_desc = D3D12_RESOURCE_DESC{};
  texture_desc.Format           = _format;
  texture_desc.Width            = width;
  texture_desc.Height           = height;
  texture_desc.DepthOrArraySize = 1;
  texture_desc.SampleDesc.Count = 1;
  texture_desc.Dimension        = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
  texture_desc.Flags            = dx12_resource_flags(_type);
  auto heap_properties          = CD3DX12_HEAP_PROPERTIES{ D3D12_HEAP_TYPE_DEFAULT };
  if (use_mipmap)
    texture_desc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
  else
    texture_desc.MipLevels = 1;

  auto clear_value = D3D12_CLEAR_VALUE{};
  clear_value.Format = texture_desc.Format;
  if (_type.contains(ImageType::dsv))
    clear_value.DepthStencil.Depth = 1.f;
  auto clear_value_ptr = (_type.any(ImageType::rtv, ImageType::dsv)) ? &clear_value : nullptr;
  err_if(device->CreateCommittedResource(&heap_properties, D3D12_HEAP_FLAG_NONE, &texture_desc, _state, clear_value_ptr, IID_PPV_ARGS(_handle.ReleaseAndGetAddressOf())),
          "failed to create image");

  create_descriptor(use_mipmap);
}

void Image::init(IDXGISwapChain1* swapchain, uint index) noexcept
{
  err_if(swapchain->GetBuffer(index, IID_PPV_ARGS(_handle.ReleaseAndGetAddressOf())),
         "failed to get descriptor");
  DXGI_SWAP_CHAIN_DESC1 desc{};
  err_if(swapchain->GetDesc1(&desc), "failed to get swapchain description");

  _width  = desc.Width;
  _height = desc.Height;
  _format = desc.Format;
  _type   = ImageType::rtv;

  create_descriptor();
}

// void Image::init(ImageType type, HANDLE handle, uint width, uint height) noexcept
// {
//   _state  = dx12_resource_state(type);
//   _width  = width;
//   _height = height;
//   err_if(g_core.device()->OpenSharedHandle(handle, IID_PPV_ARGS(_handle.ReleaseAndGetAddressOf())), "failed to share d3d11 texture");
//   create_descriptor();
// }

void Image::set_state(ID3D12GraphicsCommandList1* cmd, ImageState state) noexcept
{
  auto transition_state = static_cast<D3D12_RESOURCE_STATES>(state);
  if (_state == transition_state) return;
  auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(_handle.Get(), _state, transition_state);
  cmd->ResourceBarrier(1, &barrier);
  _state = transition_state;
}

void Image::create_descriptor(bool use_mipmap) noexcept
{
  static auto device = g_core.device();

  auto create_unordered_access_view = [&]
  {
    auto uav_desc          = D3D12_UNORDERED_ACCESS_VIEW_DESC{};
    uav_desc.Format        = _format;
    uav_desc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
    device->CreateUnorderedAccessView(_handle.Get(), nullptr, &uav_desc, _desc.uav.cpu_handle());
  };
  auto create_render_target_view = [&]
  {
    device->CreateRenderTargetView(_handle.Get(), nullptr, _desc.rtv.cpu_handle());
  };
  auto create_shader_resource_view = [&, use_mipmap = use_mipmap]
  {
    auto srv_desc = D3D12_SHADER_RESOURCE_VIEW_DESC{};
    srv_desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srv_desc.Format                  = _format;
    srv_desc.ViewDimension           = D3D12_SRV_DIMENSION_TEXTURE2D;
    srv_desc.Texture2D.MipLevels     = use_mipmap ? -1 : 1;
    device->CreateShaderResourceView(_handle.Get(), &srv_desc, _desc.srv.cpu_handle());
  };
  auto create_depth_stencil_view = [&]
  {
    auto dsv_desc = D3D12_DEPTH_STENCIL_VIEW_DESC{};
    dsv_desc.Format        = DXGI_FORMAT_D24_UNORM_S8_UINT;
    dsv_desc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
    device->CreateDepthStencilView(_handle.Get(), &dsv_desc, _desc.dsv.cpu_handle());
  };

  // first initialize image, get descriptor handle
  if (_type.contains(ImageType::srv) && !_desc.srv.is_valid())
    _desc.srv = g_desc_heap_mgr.pop_handle(DescriptorHeapType::cbv_srv_uav, create_shader_resource_view);
  if (_type.contains(ImageType::uav) && !_desc.uav.is_valid())
    _desc.uav = g_desc_heap_mgr.pop_handle(DescriptorHeapType::cbv_srv_uav, create_unordered_access_view);
  if (_type.contains(ImageType::rtv) && !_desc.rtv.is_valid())
    _desc.rtv = g_desc_heap_mgr.pop_handle(DescriptorHeapType::rtv, create_render_target_view);
  if (_type.contains(ImageType::dsv) && !_desc.dsv.is_valid())
    _desc.dsv = g_desc_heap_mgr.pop_handle(DescriptorHeapType::dsv, create_depth_stencil_view);

  // create descriptor
  if (_type.contains(ImageType::srv))
    create_shader_resource_view();
  if (_type.contains(ImageType::uav))
    create_unordered_access_view();
  if (_type.contains(ImageType::rtv))
    create_render_target_view();
  if (_type.contains(ImageType::dsv))
    create_depth_stencil_view();

  // // create mipmap uavs for generate mipmap
  // if (use_mipmap)
  // {
  //   // get count of mipmap images
  //   auto count = _handle->GetDesc().MipLevels;
  //   _mipmap_uavs.reserve(count);
  //   for (auto i : std::views::iota(1u, count))
  //   {
  //     // pop descriptor handle
  //     auto handle = g_desc_heap_mgr.pop_handle(DescriptorHeapType::cbv_srv_uav);

  //     // create mipmap uav func
  //     auto create_mipmap_uav = [this, i, handle = handle.cpu_handle()]
  //     {
  //       auto desc               = D3D12_UNORDERED_ACCESS_VIEW_DESC{};
  //       desc.Format             = _format;
  //       desc.ViewDimension      = D3D12_UAV_DIMENSION_TEXTURE2D;
  //       desc.Texture2D.MipSlice = i;
  //       device->CreateUnorderedAccessView(_handle.Get(), nullptr, &desc, handle);
  //     };
      
  //     // create uav
  //     create_mipmap_uav();

  //     // store creatation func to descriptor handle avoid dynamic expand handle invalidation
  //     handle.set(std::move(create_mipmap_uav));

  //     _mipmap_uavs.emplace_back(std::move(handle));
  //   }
  // }
}

void Image::clear(ID3D12GraphicsCommandList1* cmd, D3D12_CPU_DESCRIPTOR_HANDLE cpu_handle, D3D12_GPU_DESCRIPTOR_HANDLE gpu_handle) const noexcept
{
  err_if(!_type.contains(ImageType::uav), "clear operator only use on uav");
  float values[4]{};
  auto rect = D3D12_RECT{};
  rect.right  = _width;
  rect.bottom = _height;
  cmd->ClearUnorderedAccessViewFloat(gpu_handle, cpu_handle, _handle.Get(), values, 1, &rect);
}

void Image::clear_render_target(ID3D12GraphicsCommandList1* cmd, std::optional<Rect> rect) noexcept
{
  err_if(!_type.contains(ImageType::rtv), "clear render target only use on rtv");
  set_state(cmd, ImageState::render_target);
  float constexpr clear_color[4]{};
  if (rect)
  {
    auto rc = rect->to_RECT();
    cmd->ClearRenderTargetView(_desc.rtv.cpu_handle(), clear_color, 1, &rc);
  }
  else
    cmd->ClearRenderTargetView(_desc.rtv.cpu_handle(), clear_color, 0, nullptr);
}

void Image::clear_depth_stencil(ID3D12GraphicsCommandList1* cmd) noexcept
{
  err_if(!_type.contains(ImageType::dsv), "clear depth stencil only use on dsv");
  set_state(cmd, ImageState::depth_write);
  cmd->ClearDepthStencilView(dsv().cpu_handle(), D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.f, 0, 0, nullptr);
}

auto Image::per_pixel_size() const noexcept -> uint
{
  switch (_format)
  {
  case DXGI_FORMAT_B8G8R8A8_UNORM:
  case DXGI_FORMAT_R8G8B8A8_UNORM:
    return 4;
  default:
    err_if(true, "unsupport Image::per_pixel_size now for current format");
    std::unreachable();
  }
}

auto Image::readback(ID3D12GraphicsCommandList1* cmd, RECT rect) noexcept -> std::pair<Microsoft::WRL::ComPtr<ID3D12Resource>, Bitmap>
{
  err_if(per_pixel_size() != 4, "readback only support rgba image now");

  auto left = std::max(rect.left, 0l);
  auto top  = std::max(rect.top, 0l);

  // create bitmap view
  auto view = Bitmap{};
  view.x      = left;
  view.y      = top;
  view.width  = rect.right  - view.x;
  view.height = rect.bottom - view.y;

  // create readback buffer
  auto readback_buffer = ComPtr<ID3D12Resource>{};
  auto heap_properties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_READBACK);
  view.row_pitch       = align(view.width * per_pixel_size(), D3D12_TEXTURE_DATA_PITCH_ALIGNMENT);
  auto heap_desc       = CD3DX12_RESOURCE_DESC::Buffer(align(view.row_pitch * view.height, D3D12_TEXTURE_DATA_PLACEMENT_ALIGNMENT));
  err_if(g_core.device()->CreateCommittedResource(&heap_properties, D3D12_HEAP_FLAG_NONE, &heap_desc, D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(&readback_buffer)),
          "failed to create readback buffer");

  // get pointer of readback buffer
  auto range = D3D12_RANGE{ 0, heap_desc.Width };
  err_if(readback_buffer->Map(0, &range, reinterpret_cast<void**>(&view.data)), "failed to map readback buffer to pointer");

  // copy data from gpu to cpu
  copy(cmd, *this, view.x, view.y, rect.right, rect.bottom, readback_buffer.Get());

  return { readback_buffer, view };
}

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
  uint                        x,
  uint                        y) noexcept
{
  src.set_state(cmd, ImageState::copy_src);
  dst.set_state(cmd, ImageState::copy_dst);
  auto src_loc = CD3DX12_TEXTURE_COPY_LOCATION{ src.handle() };
  auto dst_loc = CD3DX12_TEXTURE_COPY_LOCATION{ dst.handle() };
  auto region_box = CD3DX12_BOX{ left, top, right, bottom };
  cmd->CopyTextureRegion(&dst_loc, x, y, 0, &src_loc, &region_box);
}

void copy(
  ID3D12GraphicsCommandList1* cmd,
  Image&                      src,
  LONG                        left,
  LONG                        top,
  LONG                        right,
  LONG                        bottom,
  ID3D12Resource*             readback_buffer) noexcept
{
  src.set_state(cmd, ImageState::copy_src);
  auto src_loc    = CD3DX12_TEXTURE_COPY_LOCATION{ src.handle() };
  auto region_box = CD3DX12_BOX{ left, top, right, bottom };

  auto footprint = D3D12_PLACED_SUBRESOURCE_FOOTPRINT{};
  footprint.Footprint.Width    = right - left;
  footprint.Footprint.Height   = bottom - top;
  footprint.Footprint.Depth    = 1;
  footprint.Footprint.RowPitch = align(src.per_pixel_size() * footprint.Footprint.Width, D3D12_TEXTURE_DATA_PITCH_ALIGNMENT);
  footprint.Footprint.Format   = static_cast<DXGI_FORMAT>(src.format());
  auto dst_loc = CD3DX12_TEXTURE_COPY_LOCATION{ readback_buffer, footprint };

  cmd->CopyTextureRegion(&dst_loc, 0, 0, 0, &src_loc, &region_box);
}

void copy(Bitmap const& src, Bitmap const& dst) noexcept
{
  auto src_data = reinterpret_cast<std::byte*>(src.data);
  auto dst_data = reinterpret_cast<std::byte*>(dst.data);
  for (auto i = 0; i < dst.height; ++i)
  {
    memcpy(dst_data, src_data, src.width * 4);
    src_data += src.row_pitch;
    dst_data += dst.row_pitch;
  }
}

// void Image::release_mipmap_uavs() noexcept
// {
//   assert(!_mipmap_uavs.empty());
//   for (auto& uav : _mipmap_uavs) uav.release(); 
//   _mipmap_uavs.clear();
// }

}
