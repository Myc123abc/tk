module;

#include <expected>
#include <optional>
#include <span>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

export module tk.ui.text.engine;

import tk.object_pool;
import tk.base;
import tk.hash;
import tk.rect;
import tk.singleton;
import tk.string_id;
import tk.ui;
import tk.ui.text.font;
import tk.ui.text.font_registry;
import tk.ui.text.glyph_store;
import tk.ui.text.handle;
import tk.ui.text.parse_result;
import tk.ui.text.types;

export namespace tk::ui {

constexpr auto get_text_id(StringLiteral text, std::string_view family = {}, FontStyle style = {}, TextDirection direction = {}, float tracking = {}, float limit_len = {}) noexcept
{
  return hash(text.id(), family, style, direction, tracking, limit_len);
}

// Coordinates font resolution, shaping, layout, glyph availability, and parse
// result caching. Specialized components own their respective algorithms; this
// class controls their ordering and asynchronous result lifecycle.
class TextEngine : public Singleton<TextEngine>
{
  friend class Singleton<TextEngine>;

  TextEngine() = default;
  ~TextEngine() = default;

public:
  using GlyphOrientation = tk::ui::GlyphOrientation;
  using ShapedGlyph      = tk::ui::ShapedGlyph;
  using ParseResult      = TextParseResult;

  static auto is_sideways(GlyphOrientation orientation) noexcept
  {
    return tk::ui::is_sideways(orientation);
  }

  static auto uses_vertical_shaping(uint codepoint) noexcept -> bool;

  void init() noexcept;
  void destroy() noexcept;

  auto load_font(std::string const& path) noexcept -> std::expected<FontInfo, FontLoadErrorType>;
  auto load_fonts(std::span<std::string const> paths) noexcept -> std::vector<std::expected<FontInfo, FontLoadErrorType>>;

  using ParseResultPool       = ObjectPool<ParseResult, 32, TextParseResultHandleTag>;
  using TextParseResultHandle = ui::TextParseResultHandle;

  auto parse(StringLiteral text, std::string_view family, FontStyle style, TextDirection direction, float tracking, float limit_len) noexcept -> TextParseResultHandle;

  auto& get_parse_result(TextParseResultHandle handle) const noexcept { return _parse_result_pool[handle]; }
  auto try_to_get_parse_result(uint64 text_id, uint64 fallback_id) noexcept -> std::optional<TextParseResultHandle>;

  auto get_bounding_rect(StringLiteral text, std::string_view family = {}, FontStyle style = {}, TextDirection direction = {}, float tracking = {}, float limit_len = {}) noexcept -> std::optional<Rect>;

  void update() noexcept;
  void clear_pending_copy_glyphs() noexcept;
  void postprocess() noexcept;

  auto const& get_glyph_info(GlyphKey const& key) const noexcept { return _glyph_store.info(key); }
  auto glyph_atlas(uint index) const noexcept { return _glyph_store.atlas(index); }

private:
  template <typename T>
  using TextMap               = std::unordered_map<size_t, T>;
  using ParseResultMap        = std::unordered_map<uint64, TextParseResultHandle>;
  using TextIDFallbackIDPairs = std::vector<std::pair<uint64, uint64>>;

  void queue_pending_missing_glyphs() noexcept;
  void reload_missing_glyphs() noexcept;
  void remove_missing_glyphs(FontStyleKey key) noexcept;

  FontRegistry                                                      _font_registry;
  TextShaper                                                        _shaper;
  GlyphStore                                                        _glyph_store;
  FontStyleMap<TextIDFallbackIDPairs>                               _cached_texts_with_missing_glyphs;
  ParseResultMap                                                    _cached_text_parse_results;
  TextMap<TextParseResultHandle>                                    _last_ready_text_parse_results;
  ParseResultPool                                                   _parse_result_pool;
  std::vector<TextParseResultHandle>                                _discard_text_parse_result_handles;
  std::unordered_set<TextParseResultHandle>                         _generating_results;
  std::unordered_map<TextParseResultHandle, DeferredVerticalLayout> _deferred_vertical_layouts;
};

auto& g_text_engine = TextEngine::instance();

}
