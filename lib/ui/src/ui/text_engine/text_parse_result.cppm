module;

#include <optional>
#include <string>
#include <string_view>
#include <vector>

export module tk.ui.text.parse_result;

import tk.base;
import tk.rect;
import tk.ui;
import tk.ui.text.font;
import tk.ui.text.glyph;
import tk.ui.text.types;

export namespace tk::ui {

// Mutable output of shaping and layout. The required operation order is:
// prepare_layout -> apply_tracking -> limit_length -> finalize.
// prepare_layout requires generated glyph ink metrics; finalize only measures
// and normalizes the completed layout.
struct TextParseResult
{
  std::vector<float2>           advances;
  std::vector<float2>           offsets;
  std::vector<uint>             clusters;
  std::vector<GlyphOrientation> orientations;
  float2                        extent;
  float                         ascender{};
  TextDirection                 direction{};
  float2                        layout_offset{};
  std::vector<GlyphKey>         glyph_info_keys;
  GlyphKeySet                   generating_glyph_info_keys;
  std::optional<Rect>           bounding_rect;

  TextParseResult() noexcept = default;
  TextParseResult(std::string_view text, TextDirection direction) noexcept;

  auto generate_complete() const noexcept { return generating_glyph_info_keys.empty(); }

  void emplace_back(ShapedGlyph const& glyph, uint cluster_offset, GlyphKey key) noexcept
  {
    advances.emplace_back(glyph.advance);
    offsets.emplace_back(glyph.offset);
    clusters.emplace_back(cluster_offset + glyph.cluster);
    orientations.emplace_back(glyph.orientation);
    glyph_info_keys.emplace_back(key);
    if (direction == TextDirection::horizontal)
      extent.x += glyph.advance.x;
    else
      extent.y += glyph.advance.y;
  }

  void limit_length(std::string_view text, float len) noexcept;
  void apply_tracking(float tracking) noexcept;
  void prepare_layout(GlyphKeyMap<GlyphInfo> const& glyph_infos) noexcept;
  void finalize(GlyphKeyMap<GlyphInfo> const& glyph_infos) noexcept;
};

struct DeferredVerticalLayout
{
  // Vertical ink metrics may arrive asynchronously. Preserve the unmodified
  // shaped advances so layout can be replayed in the required order.
  std::vector<float2> advances;
  float2              extent;
  std::string         text;
  float               tracking{};
  float               limit_len{};
};

auto glyph_rect(GlyphInfo const& info, float2 pos, bool sideways, bool visible) noexcept -> Rect;

}
