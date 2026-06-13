#include "image.hpp"
#include "../core.hpp"
#include "util/error_handling.hpp"
#include "command.hpp"

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
  release_mipmap_descs();
}

void Image::release_mipmap_descs() noexcept
{
  for (auto& [srv, uav] : _mipmap_descs)
  {
    srv.release();
    uav.release(); 
  }
  _mipmap_descs.clear();
}

void Image::resize(uint width, uint height) noexcept
{
  if (_width != width || _height != height) init(width, height, static_cast<ImageFormat>(_format), _types);
}

void Image::init(uint width , uint height, ImageFormat format, Flag<ImageType> type, bool use_mipmap) noexcept
{
  // not consider mipmap image can be resized
  assert(_mipmap_descs.empty());

  auto device = g_core.device();

  _width  = width;
  _height = height;
  _format = static_cast<DXGI_FORMAT>(format);
  _types  = type;
  if (_states.empty())
    _states.emplace_back(dx12_resource_states(type));
  else
    _states[0] = dx12_resource_states(type);

  // create image
  auto texture_desc = D3D12_RESOURCE_DESC{};
  texture_desc.Format           = _format;
  texture_desc.Width            = width;
  texture_desc.Height           = height;
  texture_desc.DepthOrArraySize = 1;
  texture_desc.SampleDesc.Count = 1;
  texture_desc.Dimension        = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
  texture_desc.Flags            = dx12_resource_flags(_types);
  auto heap_properties          = CD3DX12_HEAP_PROPERTIES{ D3D12_HEAP_TYPE_DEFAULT };
  if (use_mipmap)
    texture_desc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
  else
    texture_desc.MipLevels = 1;

  auto clear_value = D3D12_CLEAR_VALUE{};
  clear_value.Format = texture_desc.Format;
  if (_types.contains(ImageType::dsv))
    clear_value.DepthStencil.Depth = 1.f;
  auto clear_value_ptr = _types.any(ImageType::rtv, ImageType::dsv) ? &clear_value : nullptr;
  err_if(device->CreateCommittedResource(&heap_properties, D3D12_HEAP_FLAG_NONE, &texture_desc, _states[0], clear_value_ptr, IID_PPV_ARGS(_handle.ReleaseAndGetAddressOf())),
        "failed to create image");

  create_descriptor(use_mipmap);
}

void Image::init(IDXGISwapChain1* swapchain, uint index) noexcept
{
  assert(_mipmap_descs.empty());
  err_if(swapchain->GetBuffer(index, IID_PPV_ARGS(_handle.ReleaseAndGetAddressOf())),
         "failed to get descriptor");
  DXGI_SWAP_CHAIN_DESC1 desc{};
  err_if(swapchain->GetDesc1(&desc), "failed to get swapchain description");

  _width  = desc.Width;
  _height = desc.Height;
  _format = desc.Format;
  _types  = ImageType::rtv;

  if (_states.empty()) _states.emplace_back();

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

void Image::transform(Command const* cmd, ImageState state, uint subresource) noexcept
{
  auto transition_state = static_cast<D3D12_RESOURCE_STATES>(state);
  auto& stat = subresource == D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES ? _states[0] : _states[subresource];
  if (stat == transition_state) return;
  auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(_handle.Get(), stat, transition_state, subresource);
  cmd->get()->ResourceBarrier(1, &barrier);
  stat = transition_state;
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
  if (_types.contains(ImageType::srv) && !_desc.srv.is_valid())
    _desc.srv = g_desc_heap_mgr.pop_handle(DescriptorHeapType::cbv_srv_uav, create_shader_resource_view);
  if (_types.contains(ImageType::uav) && !_desc.uav.is_valid())
    _desc.uav = g_desc_heap_mgr.pop_handle(DescriptorHeapType::cbv_srv_uav, create_unordered_access_view);
  if (_types.contains(ImageType::rtv) && !_desc.rtv.is_valid())
    _desc.rtv = g_desc_heap_mgr.pop_handle(DescriptorHeapType::rtv, create_render_target_view);
  if (_types.contains(ImageType::dsv) && !_desc.dsv.is_valid())
    _desc.dsv = g_desc_heap_mgr.pop_handle(DescriptorHeapType::dsv, create_depth_stencil_view);

  // create descriptor
  if (_types.contains(ImageType::srv))
    create_shader_resource_view();
  if (_types.contains(ImageType::uav))
    create_unordered_access_view();
  if (_types.contains(ImageType::rtv))
    create_render_target_view();
  if (_types.contains(ImageType::dsv))
    create_depth_stencil_view();

  if (use_mipmap) create_mipmap_descs();
}

void Image::create_mipmap_descs() noexcept
{
  assert(_mipmap_descs.empty());

  auto device = g_core.device();

  // get count of mipmap images
  auto count = _handle->GetDesc().MipLevels;
  _mipmap_descs.reserve(count - 1);
  if (_states.empty())
    _states.emplace_back(static_cast<D3D12_RESOURCE_STATES>(ImageState::common));
  else
  {
    // only srv exist to generate mipmap case
    assert(_states.size() == 1);
    _states[0] = static_cast<D3D12_RESOURCE_STATES>(ImageState::common);
  }
  for (auto i = 1; i < count; ++i)
  {
    // pop descs
    auto srv = g_desc_heap_mgr.pop_handle(DescriptorHeapType::cbv_srv_uav);
    auto uav = g_desc_heap_mgr.pop_handle(DescriptorHeapType::cbv_srv_uav);

    // create mipmap descs
    auto create_mipmap_srv = [&, i, handle = srv.cpu_handle()]
    {
      auto desc = D3D12_SHADER_RESOURCE_VIEW_DESC{};
      desc.Shader4ComponentMapping   = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
      desc.Format                    = _format;
      desc.ViewDimension             = D3D12_SRV_DIMENSION_TEXTURE2D;
      desc.Texture2D.MipLevels       = 1;
      desc.Texture2D.MostDetailedMip = i - 1;
      device->CreateShaderResourceView(_handle.Get(), &desc, handle);
    };
    auto create_mipmap_uav = [&, i, handle = uav.cpu_handle()]
    {
      auto desc               = D3D12_UNORDERED_ACCESS_VIEW_DESC{};
      desc.Format             = _format;
      desc.ViewDimension      = D3D12_UAV_DIMENSION_TEXTURE2D;
      desc.Texture2D.MipSlice = i;
      device->CreateUnorderedAccessView(_handle.Get(), nullptr, &desc, handle);
    };
    
    // create srv uav
    create_mipmap_srv();
    create_mipmap_uav();

    // store creatation func to descriptor handle avoid dynamic expand handle invalidation
    g_desc_heap_mgr.set(srv, std::move(create_mipmap_srv));
    g_desc_heap_mgr.set(uav, std::move(create_mipmap_uav));

    _mipmap_descs.emplace_back(std::move(srv), std::move(uav));
    _states.emplace_back(static_cast<D3D12_RESOURCE_STATES>(ImageState::common));
  }
}

void Image::clear(Command const* cmd, D3D12_CPU_DESCRIPTOR_HANDLE cpu_handle, D3D12_GPU_DESCRIPTOR_HANDLE gpu_handle) const noexcept
{
  err_if(!_types.contains(ImageType::uav), "clear operator only use on uav");
  float values[4]{};
  auto rect = D3D12_RECT{};
  rect.right  = _width;
  rect.bottom = _height;
  cmd->get()->ClearUnorderedAccessViewFloat(gpu_handle, cpu_handle, _handle.Get(), values, 1, &rect);
}

void Image::clear_render_target(Command const* cmd, std::optional<Rect> rect) noexcept
{
  err_if(!_types.contains(ImageType::rtv), "clear render target only use on rtv");
  transform(cmd, ImageState::render_target);
  float constexpr clear_color[4]{};
  if (rect)
  {
    auto rc = rect->to_RECT();
    cmd->get()->ClearRenderTargetView(_desc.rtv.cpu_handle(), clear_color, 1, &rc);
  }
  else
    cmd->get()->ClearRenderTargetView(_desc.rtv.cpu_handle(), clear_color, 0, nullptr);
}

void Image::clear_depth_stencil(Command const* cmd, std::optional<Rect> rect) noexcept
{
  err_if(!_types.contains(ImageType::dsv), "clear depth stencil only use on dsv");
  transform(cmd, ImageState::depth_write);
  if (rect)
  {
    auto rc = rect->to_RECT();
    cmd->get()->ClearDepthStencilView(dsv().cpu_handle(), D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.f, 0, 1, &rc);
  }
  else
    cmd->get()->ClearDepthStencilView(dsv().cpu_handle(), D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.f, 0, 0, nullptr);
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

}
