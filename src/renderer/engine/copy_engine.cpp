#include "copy_engine.hpp"
#include "../resource/image.hpp"
#include "graphics_engine.hpp"

#include <algorithm>

#include <stb_image.h>

namespace tk::renderer {

////////////////////////////////////////////////////////////////////////////////
///                             Upload Buffer
////////////////////////////////////////////////////////////////////////////////

void UploadBuffer::upload(ID3D12GraphicsCommandList1* cmd) noexcept
{
  if (_infos.empty()) return;

  // calculate required intermediate sizes
  auto intermediate_sizes = std::vector<uint>{};
  intermediate_sizes.reserve(std::ranges::fold_left(_infos, 0u, [](uint cnt, auto const& info)
  {
    return cnt + info.data.visit(
      [](Bitmap const&) { return 1u; },
      [](MultiBitmapCopy const& cpy) -> uint { return cpy.infos.size(); }
    );
  }));
  for (auto const& info : _infos)
  {
    info.data.visit(
      [&](Bitmap const&)
      {
        intermediate_sizes.emplace_back(
          align(GetRequiredIntermediateSize(g_img_mgr[info.image].handle(), 0, 1), D3D12_TEXTURE_DATA_PLACEMENT_ALIGNMENT));
      },
      [&](MultiBitmapCopy const& cpys)
      {
        for (auto const& cpy : cpys.infos)
        {
          auto row_pitch = align(cpy.bitmap.row_pitch, D3D12_TEXTURE_DATA_PITCH_ALIGNMENT);
          intermediate_sizes.emplace_back(
            align(row_pitch * cpy.bitmap.height, D3D12_TEXTURE_DATA_PLACEMENT_ALIGNMENT));
        }
      }
    );
  }

  // initialize buffer
  auto size = std::ranges::fold_left(intermediate_sizes, 0, std::plus<>{});
  if (_buffer.capacity() < size)
    _buffer.init(size, false);

  // copy bitmap data to image by upload buffer
  auto offset = uint{};
  for (auto i = 0; i < _infos.size(); ++i)
  {
    auto& info = _infos[i];
    info.data.visit(
      [&](Bitmap& bitmap)
      {
        auto data = D3D12_SUBRESOURCE_DATA{};
        data.pData      = bitmap.data;
        data.RowPitch   = bitmap.row_pitch;
        data.SlicePitch = bitmap.row_pitch * bitmap.height;
        copy(cmd, g_img_mgr[info.image], _buffer, offset, data);
        if (bitmap.can_free) stbi_image_free(bitmap.data);
      },
      [&](MultiBitmapCopy const& cpys)
      {
        for (auto const& cpy : cpys.infos)
          copy(cmd, _buffer, g_img_mgr[info.image], offset, cpy.bitmap, cpy.pos);
      }
    );
    offset += intermediate_sizes[i];
  }

  _infos.clear();
}

////////////////////////////////////////////////////////////////////////////////
///                             Copy Engine
////////////////////////////////////////////////////////////////////////////////

void CopyEngine::update() noexcept
{
  auto slot = _slots.slot();

  // only process when have copy infos
  if (slot->data.empty()) return;

  // copy run, and acquire slot for next use
  _slots.slot()->data.upload(cmd());
  auto fence_value = _slots.submit_slot();
  _slots.acquire_slot();

  // wait copy complete before rendering
  g_graphics_engine.wait(g_copy_engine, fence_value);
}

}
