module;

#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

export module tk.ui.text.glyph;

import tk.base;
import tk.hash;
import tk.renderer;
import tk.ui;
import tk.ui.color;
import tk.ui.config;
import tk.ui.text.font;

export namespace tk::ui {

struct GlyphInfo
{
  uint   glyph_atlas_index{};
  float  min_x{};
  float  min_y{};
  float  max_x{};
  float  max_y{};
  float2 extent{};
  float2 pos_offset{};
  // Layout uses visible bounds while rendering still uses the full atlas rectangle.
  float2 visible_offset{};
  float2 visible_extent{};
  bool   is_color{};

  GlyphInfo() = default;

  GlyphInfo(uint glyph_atlas_index, float2 pos, float2 extent, float2 pos_offset, float2 visible_offset, float2 visible_extent, bool is_color) noexcept
    : glyph_atlas_index(glyph_atlas_index), extent(extent), pos_offset(pos_offset), visible_offset(visible_offset), visible_extent(visible_extent), is_color(is_color)
  {
    min_x = (pos.x + 0.5f) / Glyph_Atlas_Width;
    min_y = (pos.y + 0.5f) / Glyph_Atlas_Height;
    max_x = (pos.x + extent.x - 0.5f) / Glyph_Atlas_Width;
    max_y = (pos.y + extent.y - 0.5f) / Glyph_Atlas_Height;
  }

  void set_vertices(renderer::Vertex* vtx, float2 pos, float scale, float ascender, bool sideways, uint descriptor_index, Color color, Color outer_color, float outer_width) const noexcept;
};

}
