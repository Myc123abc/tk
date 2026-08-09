#include "glyph_cacher.hpp"

namespace tk::ui {

void GlyphCacher::add(PendingCopyGlyphsInfoType& info) noexcept
{
  _glyphs.reserve(_glyphs.size() + std::ranges::distance(info | std::views::values | std::views::join));
  for (auto const& [glyph_atlas_idx, bitmap_infos] : info)
  {
    for (auto const& [bitmap, pos] : bitmap_infos)
    {
      _glyphs.emplace_back
      (
        GlyphCacheHeader
        {
          bitmap.glyph_key,
          GlyphInfo{ glyph_atlas_idx, pos, bitmap.extent, bitmap.pos_offset }
        },
        std::move(bitmap.data)
      );
    }
  }
}

/*
TODO:
unsave Skyline packer data, directly use a new texture when preloading
*/
void GlyphCacher::save() noexcept
{
  
}

}
