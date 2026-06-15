#include "copy_engine.hpp"
#include "../resource/image.hpp"
#include "graphics_engine.hpp"
#include "compute_engine.hpp"
#include "../renderer.hpp"
#include "../../util/align.hpp"

#include <algorithm>

#include <stb_image.h>

namespace tk::renderer {

////////////////////////////////////////////////////////////////////////////////
///                             Upload Buffer
////////////////////////////////////////////////////////////////////////////////

void UploadBuffer::init() noexcept
{
  assert(!_buf);
  _buf = g_buf_pool.create(Buffer_Init_Size, false);
}

void UploadBuffer::destroy() noexcept
{
  assert(_buf);
  g_buf_pool.destroy(_buf);
}

void UploadBuffer::upload(Command* cmd) noexcept
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
  auto& buf = g_buf_pool[_buf];
  auto size = std::ranges::fold_left(intermediate_sizes, 0, std::plus<>{});
  if (buf.capacity() < size)
    buf.init(size, false);

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
        cmd->copy(info.image, _buf, offset, data);
        if (bitmap.can_free) stbi_image_free(bitmap.data);
      },
      [&](MultiBitmapCopy const& cpys)
      {
        for (auto const& cpy : cpys.infos)
          cmd->copy(_buf, info.image, offset, cpy.bitmap, cpy.pos);
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

  auto& [upload_buf, moved_imgs] = slot->data;

  if (upload_buf.empty() && moved_imgs.empty()) return;

  auto cmd = Engine::cmd();

  if (!upload_buf.empty()) upload_buf.upload(cmd);

  for (auto [src, dst] : moved_imgs) cmd->copy(src, dst);
  
  auto fence_value = _slots.submit_slot();

  if (cmd->needs_graphics_sync()) g_graphics_engine.wait(g_copy_engine, fence_value);
  if (cmd->needs_compute_sync()) g_comp_engine.wait(g_copy_engine, fence_value);

  if (!moved_imgs.empty())
    g_renderer.add_frame_render_complete_func([imgs = std::move(moved_imgs)] mutable
    {
      for (auto& [img, _] : imgs) g_img_mgr.destroy(img);
    }, EngineType::copy);

  if (_slots.acquire_slot()) _slots.slot()->data.init();
}

}
