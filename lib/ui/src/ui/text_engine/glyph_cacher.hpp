#pragma once

#include "text_engine.hpp"

namespace tk { struct File; }

namespace tk::ui {

struct PregenerateTextInfo
{
  std::string_view text;
  StringID         family;
  FontStyle        style{};
  TextDirection    direction{};
};

struct GlyphCacheHeader
{
  GlyphKey key;
  uint2    extent;
  float2   pos_offset;
};

struct GlyphCacheField
{
  GlyphCacheHeader   header;
  std::vector<uint8> data;
};

struct PreloadGlyphData
{
  uint2              extent;
  float2             pos_offset;
  std::vector<uint8> bitmap;
};

Singleton(GlyphCacher, g_glyph_cacher,
public:
  void init() noexcept;
  void destroy() noexcept;

  auto& preload_glyphs() noexcept
  {
    return _preload_task.is_completed() ? _preload_glyphs : _empty_preload_glyphs;
  }

  void add(PendingCopyGlyphsInfoType& info) noexcept;

  // auto pregenerate(std::span<PregenerateTextInfo const> infos) noexcept -> Task<>;
  // auto pregenerate(std::span<MSDFBitmap const> bitmaps) noexcept -> Task<>;

private:
  auto preload() noexcept -> Task<>;
  void write_cache() noexcept;
  void deserialize(std::span<uint8> data) noexcept;

private:
  std::vector<GlyphCacheField>  _glyphs;
  File*                         _file{};
  GlyphKeyMap<PreloadGlyphData> _preload_glyphs;
  GlyphKeyMap<PreloadGlyphData> _empty_preload_glyphs;
  Task<>                        _preload_task;
)

}
