module;

#include <optional>
#include <utility>
#include <vector>

export module tk.ui.text.glyph_store;

import tk.async;
import tk.base;
import tk.renderer;
import tk.ui.text.font;
import tk.ui.text.font_registry;
import tk.ui.text.glyph;
import tk.ui.text.packer;

export namespace tk::ui {

// Owns runtime glyph state: generation requests, bitmap tasks, atlas packing,
// GPU uploads, and GlyphInfo lookup. Persistent disk caching remains in
// GlyphCacher so future bin pregeneration does not depend on renderer state.
class GlyphStore
{
public:
  void init() noexcept;
  void destroy() noexcept;

  auto emplace(GlyphKey key) noexcept -> bool;
  void add_glyph_cache(GlyphKey key) noexcept;
  auto update(FontRegistry const& fonts) noexcept -> bool;
  void complete_cache_generation(FontRegistry const& fonts) noexcept;
  void cancel_cache_generation() noexcept;
  void clear_pending_copy_glyphs() noexcept;

  auto contains(GlyphKey const& key) const noexcept { return _glyph_infos.contains(key); }
  auto const& info(GlyphKey const& key) const noexcept { return _glyph_infos.at(key); }
  auto const& infos() const noexcept { return _glyph_infos; }
  auto atlas(uint index) const noexcept { return _glyph_atlas[index]; }

private:
  auto calc_glyph_pos(float2 extent) noexcept -> std::pair<uint, float2>;
  void submit_bitmap_generation_tasks(FontRegistry const& fonts) noexcept;
  void submit_cache_generation_task(FontRegistry const& fonts) noexcept;
  auto upload_bitmaps() noexcept -> bool;
  auto upload_bitmaps_from_preload(FontRegistry const& fonts) noexcept -> bool;
  void upload_bitmaps(PendingCopyGlyphsInfoType const& info) noexcept;
  static void retain_bitmaps(PendingCopyGlyphsInfoType& destination, PendingCopyGlyphsInfoType&& source) noexcept;

  std::vector<ImageHandle>                     _glyph_atlas;
  GlyphKeyMap<GlyphInfo>                       _glyph_infos;
  GlyphKeySet                                  _unuploaded_glyphs;
  GlyphKeySet                                  _ungenerated_glyphs;
  GlyphKeySet                                  _cache_ungenerated_glyphs;
  GlyphKeySet                                  _cache_generating_glyphs;
  PendingCopyGlyphsInfoType                    _pending_copy_glyphs;
  PendingCopyGlyphsInfoType                    _pending_copy_preload_glyphs;
  std::vector<Task<std::vector<MSDFBitmap>>>   _generate_bitmap_tasks;
  std::optional<Task<std::vector<MSDFBitmap>>> _generate_cache_task;
  SkylinePacker                                _packer;
  uint                                         _current_glyph_atlas_idx{};
};

}
