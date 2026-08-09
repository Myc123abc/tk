#pragma once

#include "tk/base.hpp"
#include "../config.hpp"
#include "../../renderer/resource/shader_type.hpp"
#include "ui/color.hpp"

#include <unordered_map>
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
using GlyphKeyMap = std::unordered_map<GlyphKey, T, GlyphKeyHash>;
using GlyphKeySet = std::unordered_set<GlyphKey, GlyphKeyHash>;

struct GlyphInfo
{
  uint   glyph_atlas_index{};
  float  min_x{};
  float  min_y{};
  float  max_x{};
  float  max_y{};
  float2 extent{};
  float2 pos_offset{};

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
