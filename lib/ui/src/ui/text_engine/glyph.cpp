module tk.ui.text.glyph;

using namespace tk::renderer;

namespace tk::ui {

void GlyphInfo::set_vertices(renderer::Vertex* vtx, float2 pos, float scale, float ascender, bool sideways, uint descriptor_index, Color color, Color outer_color, float outer_width) const noexcept
{
  auto p0 = float2{};
  auto p1 = float2{};
  auto p2 = float2{};
  auto p3 = float2{};
  if (sideways)
  {
    auto const offset       = pos_offset * scale;
    auto const glyph_extent = extent     * scale;
    p0 = pos + float2{ -offset.y,                  offset.x                  };
    p1 = pos + float2{ -offset.y,                  offset.x + glyph_extent.x };
    p2 = pos + float2{ -offset.y - glyph_extent.y, offset.x + glyph_extent.x };
    p3 = pos + float2{ -offset.y - glyph_extent.y, offset.x                  };
  }
  else
  {
    p0 = pos + pos_offset * scale;
    p0.y += ascender * scale;
    p1 = float2{ p0.x + extent.x * scale, p0.y };
    p3 = float2{ p0.x, p0.y + extent.y * scale };
    p2 = float2{ p1.x, p3.y };
  }

  auto col       = color.to_uint();
  auto outer_col = outer_color.to_uint();
  auto packed    = vtx_pack(is_color ? VtxType::color_text : VtxType::text, descriptor_index);
  vtx[0] = { p0, { min_x, min_y }, col, packed, outer_col, outer_width };
  vtx[1] = { p1, { max_x, min_y }, col, packed, outer_col, outer_width };
  vtx[2] = { p2, { max_x, max_y }, col, packed, outer_col, outer_width };
  vtx[3] = { p3, { min_x, max_y }, col, packed, outer_col, outer_width };
}

}
