#include "copy_engine.hpp"
#include "../resource/image.hpp"
#include "graphics_engine.hpp"
#include "compute_engine.hpp"
#include "../renderer.hpp"
#include "../../util/align.hpp"
#include "../../ui/text_engine.hpp"

#include <algorithm>

#include <stb_image.h>

namespace tk::renderer {

using namespace ui;

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
          auto row_pitch = align(cpy.bitmap_view.row_pitch, D3D12_TEXTURE_DATA_PITCH_ALIGNMENT);
          intermediate_sizes.emplace_back(
            align(row_pitch * cpy.bitmap_view.height, D3D12_TEXTURE_DATA_PLACEMENT_ALIGNMENT));
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
  for (auto i = 0; i < _infos.size(); ++i)
  {
    auto& info = _infos[i];
    info.data.visit(
      [&](Bitmap& bitmap)
      {
        cmd->copy(_buf, info.image, bitmap);
        if (bitmap.can_free) stbi_image_free(bitmap.data);
      },
      [&](MultiBitmapCopy const& cpys)
      {
        cmd->copy(_buf, info.image, cpys.infos);
      }
    );
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

  // remove glyphs after copy finished
  auto& pending_copy_glyphs = g_text_engine.access_swaped_pending_copy_glyphs();
  if (!pending_copy_glyphs.empty())
    g_renderer.add_frame_render_complete_func([&] { pending_copy_glyphs.clear(); }, EngineType::copy);

  if (cmd->needs_graphics_sync()) g_graphics_engine.wait(g_copy_engine, fence_value);
  if (cmd->needs_compute_sync()) g_comp_engine.wait(g_copy_engine, fence_value);
  cmd->clear_resource_track();

  if (!moved_imgs.empty())
    g_renderer.add_frame_render_complete_func([imgs = std::move(moved_imgs)] mutable
    {
      for (auto& [img, _] : imgs) g_img_mgr.destroy(img);
    }, EngineType::copy);

  if (_slots.acquire_slot()) _slots.slot()->data.init();
}

}
