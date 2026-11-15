#pragma once

#include "glyph.hpp"
#include "packer.hpp"
#include "../../renderer/resource/image_manager.hpp"
#include "../../util/async/task.hpp"

#include <utility>
#include <vector>

namespace tk::ui {

class FontRegistry;

// Owns runtime glyph state: generation requests, bitmap tasks, atlas packing,
// GPU uploads, and GlyphInfo lookup. Persistent disk caching remains in
// GlyphCacher so future bin pregeneration does not depend on renderer state.
class GlyphStore
{
public:
  void init() noexcept;
  void destroy() noexcept;

  auto request(GlyphKey key) noexcept -> bool;
  auto update(FontRegistry const& fonts) noexcept -> bool;
  void clear_pending_copy_glyphs() noexcept;

  auto contains(GlyphKey const& key) const noexcept { return _glyph_infos.contains(key); }
  auto const& info(GlyphKey const& key) const noexcept { return _glyph_infos.at(key); }
  auto const& infos() const noexcept { return _glyph_infos; }
  auto atlas(uint index) const noexcept { return _glyph_atlas[index]; }

private:
  auto calc_glyph_pos(float2 extent) noexcept -> std::pair<uint, float2>;
  void submit_bitmap_generation_tasks(FontRegistry const& fonts) noexcept;
  auto upload_bitmaps() noexcept -> bool;
  auto upload_bitmaps_from_preload(FontRegistry const& fonts) noexcept -> bool;
  void upload_bitmaps(PendingCopyGlyphsInfoType const& info) noexcept;

  std::vector<ImageHandle>                   _glyph_atlas;
  GlyphKeyMap<GlyphInfo>                     _glyph_infos;
  GlyphKeySet                                _uncached_glyphs;
  GlyphKeySet                                _ungenerated_glyphs;
  PendingCopyGlyphsInfoType                  _pending_copy_glyphs;
  PendingCopyGlyphsInfoType                  _pending_copy_preload_glyphs;
  std::vector<Task<std::vector<MSDFBitmap>>> _generate_bitmap_tasks;
  SkylinePacker                              _packer;
  uint                                       _current_glyph_atlas_idx{};
};

}
