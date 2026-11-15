#pragma once

#include "font.hpp"
#include "glyph.hpp"
#include "text_types.hpp"

#include <array>
#include <expected>
#include <memory>
#include <span>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace tk::ui {

struct ResolvedFontRun
{
  std::string_view text;
  Font*            font{};
};

struct ResolvedText
{
  std::vector<ResolvedFontRun> runs;
  TextLineMetrics              metrics;
  bool                         has_missing_glyphs{};
};

// Owns loaded fonts and converts grapheme clusters into contiguous font runs.
// Emoji presentation, style fallback, and late-loaded missing glyphs are
// resolved here so shaping never needs to search the font collection.
class FontRegistry
{
public:
  auto load_font(std::string const& path) noexcept -> std::expected<FontInfo, FontLoadErrorType>;
  auto load_fonts(std::span<std::string const> paths) noexcept -> std::vector<std::expected<FontInfo, FontLoadErrorType>>;

  auto resolve(std::string_view text, FontStyleKey key) noexcept -> ResolvedText;
  auto notdef_font() noexcept -> Font*;
  auto font(FontStyleKey key) const noexcept -> Font*;

  // Glyphs recovered by a newly loaded fallback font. TextEngine queues these
  // in GlyphStore and invalidates affected parse results after generation.
  auto& pending_missing_glyphs() noexcept { return _pending_missing_glyphs; }
  void complete_missing_glyph_reload(FontStyleKey key) noexcept;
  void clear() noexcept;

private:
  using StyleFontIdxs      = std::array<std::vector<uint>, static_cast<size_t>(FontStyle::italic_bold) + 1>;
  using FallbackFontIdxMap = FontStyleMap<std::unordered_map<uint, uint>>;

  auto install_font(std::unique_ptr<Font> font) noexcept -> FontInfo;
  auto find_font(uint unicode, FontStyleKey key) noexcept -> Font*;
  void regenerate_missing_glyphs(Font* font, FontStyleKey key) noexcept;

  Font*                                  _notdef_font{};
  std::vector<std::unique_ptr<Font>>     _fonts;
  FontStyleMap<uint>                     _font_idxs;
  StyleFontIdxs                          _style_font_idxs;
  FontStyleMap<std::unordered_set<uint>> _missing_glyphs;
  FallbackFontIdxMap                     _fallback_font_idxs;
  FontStyleMap<GlyphKeySet>              _pending_missing_glyphs;
};

}
