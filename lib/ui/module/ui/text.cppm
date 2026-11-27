module;

#include <string>
#include <expected>
#include <span>
#include <vector>
#include <optional>

export module tk.ui:text;

import tk.ui.color;
import tk.base;
import tk.string_id;
import tk.variant;
import tk.rect;

export namespace tk::ui {

enum class FontStyle
{
  regular,
  italic,
  bold,
  italic_bold,
};

enum class TextDirection
{
  horizontal,
  vertical,
};

struct FontInfo
{
  std::string family;
  FontStyle   style;
};

namespace FontLoadError {

struct unexist {};
struct freetype_err { uint8 code{}; };

using Type = Variant<unexist, freetype_err>;

}

using FontLoadErrorType = FontLoadError::Type;

/**
 * load font
 * @param path
 * @return font info
 */
auto load_font(std::string const& path) noexcept -> std::expected<FontInfo, FontLoadErrorType>;

/**
 * load fonts in parallel
 * @param paths font paths
 * @return font info or error for each input path, in input order
 */
auto load_fonts(std::span<std::string const> paths) noexcept -> std::vector<std::expected<FontInfo, FontLoadErrorType>>;

struct TextConfig
{
  std::string_view family;
  FontStyle        style{};
  TextDirection    direction{};
  Color            outer_color;
  float            outline_width{ 0.15f };
  bool             pos_as_baseline{};
  float            tracking{};  // Additional spacing between grapheme clusters.
  float            limit_len{}; // 0 disables wrapping; positive limits are clamped to the widest grapheme cluster.
};

struct TextResult
{
  float2 extent;
  float  ascender{};
};

/**
 * draw text
 * @param text
 * @param pos left top of text
 * @param size
 * @param color
 * @param cfg text config
 * @return text parse result
 */
auto text(StringLiteral text, float2 pos, float size, Color color, TextConfig const& cfg = {}) noexcept -> TextResult;

/**
 * get parse result of text with sepcific config
 * @param text
 * @param size
 * @param cfg text config
 * @return text parse result
 */
auto text(StringLiteral text, float size, TextConfig const& cfg = {}) noexcept -> TextResult;

/**
 * Get bounding rectangle of the text in specific size.
 * @param text
 * @param size
 * @param cfg text config
 * @return bounding rectangle if text generation complete.
 */
auto get_text_bounding_rect(StringLiteral text, float size, TextConfig const& cfg = {}) noexcept -> std::optional<Rect>;

/**
 * Queue low-priority disk-cache generation for glyphs produced by UTF-8 text.
 * Visible text generation takes priority, and generated glyphs are not uploaded
 * to the atlas until rendered.
 * @param path font path
 * @param chars UTF-8 encoded text
 */
void generate_glyphs_cache(std::string_view path, std::string_view chars) noexcept;

/**
 * Queue low-priority disk-cache generation for glyphs produced by UTF-8 bytes.
 * @param path font path
 * @param chars UTF-8 encoded text
 */
void generate_glyphs_cache(std::string_view path, std::span<uint8> chars) noexcept;

enum class Language
{
  en,
  zh_cn,
  zh_tw,
  jp,
  ko,
  ru,
};

/**
 * Queue low-priority disk-cache generation for supported common characters.
 * Traditional Chinese uses CLDR 48's primary and auxiliary exemplars for Taiwan.
 * Russian includes the 33 uppercase and lowercase letters of the Russian alphabet.
 * Korean includes compatibility jamo and the 2,350 KS X 1001 Hangul syllables.
 * @param path font path
 * @param langs languages to include
 */
void generate_glyphs_cache(std::string_view path, std::span<Language const> langs) noexcept;

/**
 * Complete every queued glyph-cache request and persist glyphs.bin.
 * Intended for offline asset generation, not an interactive render loop.
 */
void complete_glyphs_cache_generation() noexcept;

}
