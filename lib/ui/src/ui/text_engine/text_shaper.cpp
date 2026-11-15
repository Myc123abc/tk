#include "text_shaper.hpp"
#include "vertical_orientation.hpp"
#include "grapheme_iterator.hpp"

namespace tk::ui {

void TextShaper::init() noexcept
{
  _buffer = hb_buffer_create();
}

void TextShaper::destroy() noexcept
{
  hb_buffer_destroy(_buffer);
  _buffer = {};
}

auto TextShaper::shape_text(std::string_view text, Font* font, TextDirection direction, ShapingMode mode, TextLineMetrics metrics) noexcept -> std::vector<ShapedGlyph>
{
  assert(font);
  assert(!text.empty());
  assert(mode == ShapingMode::normal || direction == TextDirection::vertical);

  auto result = std::vector<ShapedGlyph>{};
  std::lock_guard lock{ font->_mutex };
  hb_buffer_reset(_buffer);
  hb_buffer_add_utf8(_buffer, text.data(), static_cast<int>(text.size()), 0, -1);
  hb_buffer_set_cluster_level(_buffer, HB_BUFFER_CLUSTER_LEVEL_MONOTONE_GRAPHEMES);
  if (mode != ShapingMode::normal)
    hb_buffer_set_direction(_buffer, HB_DIRECTION_LTR);
  else if (direction == TextDirection::vertical)
    hb_buffer_set_direction(_buffer, HB_DIRECTION_TTB);
  hb_buffer_guess_segment_properties(_buffer);
  hb_shape(font->_hb_font, _buffer, nullptr, 0);

  auto const glyph_count     = hb_buffer_get_length(_buffer);
  auto const glyph_infos     = hb_buffer_get_glyph_infos(_buffer, nullptr);
  auto const glyph_positions = hb_buffer_get_glyph_positions(_buffer, nullptr);
  result.reserve(glyph_count);
  for (auto i = 0u; i < glyph_count; ++i)
  {
    auto advance = float2
    {
      static_cast<float>(glyph_positions[i].x_advance) / 64,
      static_cast<float>(glyph_positions[i].y_advance) / 64,
    };
    auto offset = float2
    {
      static_cast<float>(glyph_positions[i].x_offset) / 64,
      static_cast<float>(glyph_positions[i].y_offset) / 64,
    };
    if (direction == TextDirection::vertical)
    {
      if (mode == ShapingMode::normal)
      {
        advance.y = -advance.y;
        offset.y  = -offset.y;
      }
      else if (mode == ShapingMode::sideways || mode == ShapingMode::sideways_centered)
      {
        advance = { 0.f, advance.x };
        offset  = { sideways_offset_x(offset.y, metrics.ascender, metrics.height), offset.x };
      }
    }
    result.emplace_back(glyph_infos[i].codepoint, advance, offset, glyph_infos[i].cluster,
      mode == ShapingMode::sideways || mode == ShapingMode::sideways_centered
        ? mode == ShapingMode::sideways_centered
          ? GlyphOrientation::sideways_centered
          : GlyphOrientation::sideways
        : mode == ShapingMode::horizontal_in_vertical
          ? GlyphOrientation::upright_composed
          : GlyphOrientation::upright);
  }

  if (mode == ShapingMode::horizontal_in_vertical)
  {
    auto run_advance = 0.f;
    for (auto const& glyph : result)
      run_advance += glyph.advance.x;

    auto pen_x = 0.f;
    for (auto& glyph : result)
    {
      glyph.offset.x += pen_x - run_advance * .5f;
      glyph.offset.y += font->_ascender;
      pen_x          += glyph.advance.x;
      glyph.advance  = {};
    }
    result.back().advance.y = font->_height;
  }
  return result;
}

auto TextShaper::shape(std::string_view text, Font* font, TextDirection direction, TextLineMetrics metrics) noexcept -> std::vector<ShapedGlyph>
{
  if (direction != TextDirection::vertical)
    return shape_text(text, font, direction, ShapingMode::normal, metrics);

  // HarfBuzz direction applies to the whole buffer. Split vertical text into
  // runs that need TTB shaping, rotated horizontal shaping, or one-cell emoji
  // composition before submitting each run separately.
  auto result = std::vector<ShapedGlyph>{};
  auto append = [&](std::string_view shaped_text, ShapingMode mode)
  {
    auto glyphs         = shape_text(shaped_text, font, direction, mode, metrics);
    auto cluster_offset = static_cast<uint>(shaped_text.data() - text.data());
    for (auto& glyph : glyphs)
    {
      glyph.cluster += cluster_offset;
      result.emplace_back(std::move(glyph));
    }
  };

  auto grapheme_iterator   = GraphemeIterator(text);
  auto is_sideways_cluster = [](std::string_view cluster)
  {
    for (auto index = 0u; index < cluster.size();)
    {
      auto const [codepoint, size, valid] = next_codepoint(cluster, index);
      assert(valid);
      if (uses_vertical_shaping(codepoint))
        return false;
      index += size;
    }
    return true;
  };
  auto cluster_mode = [&](std::string_view cluster)
  {
    if (font->has_color_glyphs() && codepoint_cnt(cluster) > 1)
      return ShapingMode::horizontal_in_vertical;
    if (!is_sideways_cluster(cluster)) return ShapingMode::normal;

    auto const codepoint = next_codepoint(cluster, 0).codepoint;
    auto const category  = hb_unicode_general_category(hb_unicode_funcs_get_default(), codepoint);
    auto const centered  = category == HB_UNICODE_GENERAL_CATEGORY_OPEN_PUNCTUATION
                        || category == HB_UNICODE_GENERAL_CATEGORY_CLOSE_PUNCTUATION
                        || codepoint == '<' || codepoint == '>';
    return centered ? ShapingMode::sideways_centered : ShapingMode::sideways;
  };

  auto run_begin = grapheme_iterator.begin();
  auto run_end   = grapheme_iterator.next(run_begin);
  auto run_mode  = cluster_mode(grapheme_iterator.grapheme(run_begin, run_end));
  if (run_mode == ShapingMode::horizontal_in_vertical)
  {
    append(grapheme_iterator.grapheme(run_begin, run_end), run_mode);
    run_begin = run_end;
  }

  while (run_end < grapheme_iterator.end())
  {
    auto const end  = grapheme_iterator.next(run_end);
    auto const mode = cluster_mode(grapheme_iterator.grapheme(run_end, end));
    if (mode == ShapingMode::horizontal_in_vertical)
    {
      if (run_begin < run_end)
        append(grapheme_iterator.grapheme(run_begin, run_end), run_mode);
      append(grapheme_iterator.grapheme(run_end, end), mode);
      run_begin = end;
    }
    else if (run_begin == run_end)
    {
      run_begin = run_end;
      run_mode  = mode;
    }
    else if (mode != run_mode)
    {
      append(grapheme_iterator.grapheme(run_begin, run_end), run_mode);
      run_begin = run_end;
      run_mode  = mode;
    }
    run_end = end;
  }
  if (run_begin < grapheme_iterator.end())
    append(grapheme_iterator.grapheme(run_begin, run_end), run_mode);

  return result;
}

auto TextShaper::shape_notdef(std::string_view text, Font* font, TextDirection direction, TextLineMetrics metrics) noexcept -> std::vector<ShapedGlyph>
{
  return shape_text(text, font, direction, ShapingMode::normal, metrics);
}

}
