#pragma once

#include "ui/ui.hpp"
#include "../../renderer/resource/image.hpp"

#include <ft2build.h>
#include FT_FREETYPE_H
#include <hb.h>

#include <string>
#include <mutex>
#include <unordered_set>

namespace tk::ui {

struct GlyphKey
{
  friend struct GlyphKeyHash;

  GlyphKey() noexcept = default;
  GlyphKey(uint font_id, uint unicode) noexcept : _k(static_cast<uint64>(font_id) << 32 | unicode) {}

  auto operator==(GlyphKey const&) const noexcept -> bool = default;

  auto font_id() const noexcept -> uint { return _k >> 32; }
  auto codepoint() const noexcept -> uint { return _k & 0xffffffff; }

private:
  uint64 _k{};
};

struct GlyphKeyHash
{
  auto operator()(GlyphKey k) const noexcept
  {
    return std::hash<uint64>{}(k._k);
  }
};

template <typename T>
using GlyphKeyMap  = std::unordered_map<GlyphKey, T, GlyphKeyHash>;
using GlyphKeySet  = std::unordered_set<GlyphKey, GlyphKeyHash>;

struct MSDFBitmap
{
  std::vector<uint8> data;
  uint2              extent{};
  float2             pos_offset{};
  GlyphKey           glyph_key;

  auto to_bitmap_view() const noexcept -> renderer::BitmapView
  {
    return { data.data(), extent.x, extent.y, extent.x * 4 };
  }

  auto empty() const noexcept { return !extent.x || !extent.y; }
};

class Font
{
  friend class TextEngine;
public:
  auto init(std::string_view path) noexcept -> uint8;
  void destroy() const noexcept;

  auto name()  const noexcept { return _name;  }
  auto style() const noexcept { return _style; }

  auto find_glyph(uint unicode) const noexcept
  {
    std::lock_guard lock{ _mutex };
    return FT_Get_Char_Index(_face, unicode);
  }

  auto get_glyph_advance(uint glyph_idx) const noexcept -> float2;
  auto generate_msdf_bitmap(uint glyph_idx, GlyphKey key) const noexcept -> MSDFBitmap;

  auto id() const noexcept { return _id; }

private:
  uint        _id{};
  std::string _name;
  std::string _family;
  FontStyle   _style;
  FT_Face     _face{};
  hb_font_t*  _hb_font{};
  float       _ascender{};
  float       _height{};
  mutable std::mutex _mutex;
};

}
