#pragma once

#include "font.hpp"
#include "text_types.hpp"

#include <string_view>
#include <vector>

namespace tk::ui {

// Shapes already-resolved font runs. It owns the reusable HarfBuzz buffer and
// applies vertical orientation without performing fallback or bitmap lookup.
class TextShaper
{
public:
  void init() noexcept;
  void destroy() noexcept;

  auto shape(std::string_view text, Font* font, TextDirection direction, TextLineMetrics metrics) noexcept -> std::vector<ShapedGlyph>;

  // Preserve the legacy notdef path: missing glyph runs use normal shaping
  // instead of being split into upright and sideways vertical runs.
  auto shape_notdef(std::string_view text, Font* font, TextDirection direction, TextLineMetrics metrics) noexcept -> std::vector<ShapedGlyph>;

private:
  auto shape_text(std::string_view text, Font* font, TextDirection direction, ShapingMode mode, TextLineMetrics metrics) noexcept -> std::vector<ShapedGlyph>;

  hb_buffer_t* _buffer{};
};

}
