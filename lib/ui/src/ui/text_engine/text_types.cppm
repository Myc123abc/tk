export module tk.ui.text.types;

import tk.base;

export namespace tk::ui {

enum class GlyphOrientation : uint8
{
  // HarfBuzz TTB shaping; used by CJK and vertical-form punctuation.
  upright,
  // Horizontally shaped compound rendered as one upright vertical cell.
  upright_composed,
  // Horizontally shaped glyph rotated clockwise in vertical text.
  sideways,
  // Sideways glyph centered in the vertical cell, primarily punctuation.
  sideways_centered,
};

constexpr auto is_sideways(GlyphOrientation orientation) noexcept
{
  return orientation == GlyphOrientation::sideways
      || orientation == GlyphOrientation::sideways_centered;
}

enum class ShapingMode : uint8
{
  normal,
  horizontal_in_vertical,
  sideways,
  sideways_centered,
};

struct ShapedGlyph
{
  uint             glyph_index{};
  float2           advance;
  float2           offset;
  uint             cluster{};
  GlyphOrientation orientation{};
};

struct TextLineMetrics
{
  float ascender{};
  float height{};
};

}
