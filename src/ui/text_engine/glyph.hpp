#pragma once

#include "tk/base.hpp"
#include "../config.hpp"
#include "font.hpp"
#include "../../renderer/resource/image.hpp"
#include "../../renderer/resource/shader_type.hpp"
#include "ui/color.hpp"

#include <unordered_map>
#include <unordered_set>

namespace tk::ui {

struct GlyphKey
{
  GlyphKey() noexcept = default;
  GlyphKey(FontStyleKey const& font_key, uint glyph_index) noexcept
    : font_key(font_key), glyph_index(glyph_index) {}

  auto operator==(GlyphKey const&) const noexcept -> bool = default;

  FontStyleKey font_key;
  uint         glyph_index{};
};

struct GlyphKeyHash
{
  auto operator()(GlyphKey const& key) const noexcept
  {
    return generic_hash(FontStyleKeyHash{}(key.font_key), key.glyph_index);
  }
};

template <typename T>
using GlyphKeyMap = std::unordered_map<GlyphKey, T, GlyphKeyHash>;
using GlyphKeySet = std::unordered_set<GlyphKey, GlyphKeyHash>;

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

struct GlyphInfo
{
  uint   glyph_atlas_index{};
  float  min_x{};
  float  min_y{};
  float  max_x{};
  float  max_y{};
  float2 extent{};
  float2 pos_offset{};

  GlyphInfo() = default;

  GlyphInfo(uint glyph_atlas_index, float2 pos, float2 extent, float2 pos_offset) noexcept
    : glyph_atlas_index(glyph_atlas_index), extent(extent), pos_offset(pos_offset)
  {
    min_x = (pos.x + 0.5f) / Glyph_Atlas_Width;
    min_y = (pos.y + 0.5f) / Glyph_Atlas_Height;
    max_x = (pos.x + extent.x - 0.5f) / Glyph_Atlas_Width;
    max_y = (pos.y + extent.y - 0.5f) / Glyph_Atlas_Height;
  }

  static auto get_scale(float size) noexcept
  {
    return size / FT_Pixel_Size;
  }

  void set_vertices(renderer::Vertex* vtx, float2 pos, float size, float ascender, Color color, Color outer_color, float outer_width) const noexcept;

  static auto get_next_position(float2 pos, float size, float2 advance) noexcept
  {
    return pos + advance * get_scale(size);
  }
};

}
