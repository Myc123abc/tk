#pragma once

#include "text_engine.hpp"

namespace tk::ui {

/*
TODO:
can use zstd to partition compressing cache files
*/

struct GlyphCacheHeader
{
  GlyphKey  key;
  GlyphInfo info;
};

struct GlyphCacheData
{
  GlyphCacheHeader   header;
  std::vector<uint8> bitmap;
};

Singleton(GlyphCacher, g_glyph_cacher,

public:
  void add(PendingCopyGlyphsInfoType& info) noexcept;
  void save() noexcept;

private:
  std::vector<GlyphCacheData> _glyphs;
)

}
