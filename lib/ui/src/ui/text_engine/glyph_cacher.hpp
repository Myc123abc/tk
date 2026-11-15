#pragma once

#include "glyph.hpp"
#include "../../util/async/task.hpp"
#include "../../util/singleton.hpp"

namespace tk { struct File; }

namespace tk::ui {

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

// Persistent cache for generated monochrome glyph bitmaps. Preload is
// asynchronous, but generation must wait for it to complete so cached keys are
// not regenerated and appended again.
Singleton(GlyphCacher, g_glyph_cacher,
public:
  void init() noexcept;
  void destroy() noexcept;

  auto preload_complete() const noexcept
  {
    return _preload_task.is_completed();
  }

  auto& preload_glyphs() noexcept
  {
    assert(preload_complete());
    return _preload_glyphs;
  }

  void store(PendingCopyGlyphsInfoType& info) noexcept;

private:
  auto preload() noexcept -> Task<>;
  void write_cache() noexcept;
  void deserialize(std::span<uint8> data) noexcept;

private:
  std::vector<GlyphCacheField>  _glyphs;
  File*                         _file{};
  GlyphKeyMap<PreloadGlyphData> _preload_glyphs;
  GlyphKeySet                   _cached_glyph_keys;
  Task<>                        _preload_task;
)

}
