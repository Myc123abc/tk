#pragma once

#include "text_engine.hpp"

namespace tk { struct File; }

namespace tk::ui {

/*
TODO:
can use zstd to partition compressing cache files
*/

struct GlyphCacheHeader
{
  GlyphKey key;
  uint2    extent;
  float2   pos_offset;
  uint     bitmap_size{};
};

struct GlyphCacheData
{
  GlyphCacheHeader   header;
  std::vector<uint8> bitmap;
};

struct PreloadGlyphData
{
  uint2              extent;
  float2             pos_offset;
  std::vector<uint8> bitmap;
};

Singleton(GlyphCacher, g_glyph_cacher,
public:
  void add(PendingCopyGlyphsInfoType& info) noexcept;
  void destroy() noexcept;

  void preload() noexcept;
  void wait_preload_complete() noexcept;

  auto& preload_glyphs() noexcept { return _preload_glyphs; }

private:
  auto serialize() const noexcept -> std::vector<uint8>;
  void deserialize(std::span<uint8 const> data) noexcept;

private:
  std::vector<GlyphCacheData>   _glyphs;
  File*                         _file{};
  GlyphKeyMap<PreloadGlyphData> _preload_glyphs;
)

}
