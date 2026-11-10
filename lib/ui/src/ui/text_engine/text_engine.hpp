#pragma once

#include "../../util/object_pool.hpp"
#include "../../renderer/resource/image_manager.hpp"
#include "../../util/async/task.hpp"
#include "font.hpp"
#include "glyph.hpp"
#include "packer.hpp"

namespace tk::ui {

using PendingCopyGlyphsInfoType = std::unordered_map<uint, std::vector<std::pair<MSDFBitmap, float2>>>;

constexpr auto get_text_id(
  StringLiteral    text,
  std::string_view family    = {},
  FontStyle        style     = {},
  TextDirection    direction = {},
  float           limit_len  = {}) noexcept
{
  return hash(text.id(), family, style, direction, limit_len);
}

Singleton(TextEngine, g_text_engine,
  friend class Font;
  friend class GlyphInfo;
public:
  void init() noexcept;
  void destroy() noexcept;

  auto load_font(std::string const& path) noexcept -> std::expected<FontInfo, FontLoadErrorType>;
  auto load_fonts(std::span<std::string const> paths) noexcept -> std::vector<std::expected<FontInfo, FontLoadErrorType>>;

  struct ShapedGlyph
  {
    uint   glyph_index{};
    float2 advance;
    float2 offset;
    uint   cluster{};
  };

  struct ParseResult
  {
    std::vector<float2>   advances;
    std::vector<float2>   offsets;
    std::vector<uint>     clusters;
    float2                extent;
    float                 ascender{};
    TextDirection         direction{};
    float2                layout_offset{};
    std::vector<GlyphKey> glyph_info_keys;
    GlyphKeySet           generating_glyph_info_keys;
    std::optional<Rect>   bounding_rect;

    ParseResult() noexcept = default;

    auto generate_complete() const noexcept { return generating_glyph_info_keys.empty(); }

    void emplace_back(ShapedGlyph const& glyph, uint cluster_offset, GlyphKey key) noexcept
    {
      advances.emplace_back(glyph.advance);
      offsets.emplace_back(glyph.offset);
      clusters.emplace_back(cluster_offset + glyph.cluster);
      glyph_info_keys.emplace_back(key);
      if (direction == TextDirection::horizontal)
        extent.x += glyph.advance.x;
      else
        extent.y += glyph.advance.y;
    }

    void limit_length(std::string_view text, float len) noexcept;
  };
  using ParseResultPool       = ObjectPool<ParseResult>;
  using TextParseResultHandle = ParseResultPool::Handle;
  auto parse(StringLiteral text, std::string_view family, FontStyle style, TextDirection direction, float limit_len) noexcept -> TextParseResultHandle;
  auto& get_parse_result(TextParseResultHandle handle) const noexcept { return _parse_result_pool[handle]; }
  auto try_to_get_parse_result(uint64 text_id, uint64 text_hash) noexcept -> std::optional<TextParseResultHandle>;

  auto get_bounding_rect(StringLiteral text, std::string_view family = {}, FontStyle style = {}, TextDirection direction = {}, float limit_len = {}) noexcept -> std::optional<Rect>;

  void update() noexcept;

  void clear_pending_copy_glyphs() noexcept;
  auto const& get_glyph_info(GlyphKey const& key) const noexcept { return _glyph_infos.at(key); }
  auto glyph_atlas(uint index) const noexcept { return _glyph_atlas[index]; }

  void postprocess() noexcept;

private:
  auto install_font(std::unique_ptr<Font> font) noexcept -> FontInfo;
  auto shape_text(std::string_view text, Font* font, TextDirection direction, bool horizontal_in_vertical = false) noexcept -> std::vector<ShapedGlyph>;
  auto split_text(std::string_view text, FontStyleKey key) noexcept -> std::vector<std::pair<std::string_view, Font*>>;
  auto find_font(uint unicode, FontStyleKey key) noexcept -> Font*;
  auto find_notdef_glyph_font() noexcept -> Font*;
  void finalize_parse_result(ParseResult& result) noexcept;
  void add_uncached_glyph(Font* font, uint glyph_idx, GlyphKey const& key, ParseResult& result) noexcept;
  auto add_notdef_glyph() noexcept -> bool;
  auto calc_glyph_pos(float2 extent) noexcept -> std::pair<uint, float2>;
  void regenerate_missing_glyphs(Font* font, FontStyleKey key) noexcept;
  void remove_missing_glyphs(FontStyleKey key) noexcept;
  void reload_missing_glyphs() noexcept;

  void submit_bitmap_generation_tasks() noexcept;
  auto upload_bitmaps() noexcept -> bool;
  auto upload_bitmaps_from_preload_glyphs() noexcept -> bool;
  void upload_bitmaps(PendingCopyGlyphsInfoType const& info) noexcept;

private:
  template <typename T>
  using TextMap             = std::unordered_map<size_t, T>;
  using ParseResultMap      = std::unordered_map<uint64, TextParseResultHandle>;
  using StyleFontIdxs       = std::array<std::vector<uint>, static_cast<size_t>(FontStyle::italic_bold) + 1>;
  using FallbackFontIdxMap  = FontStyleMap<std::unordered_map<uint, uint>>;
  using TextIDTextHashPairs = std::vector<std::pair<uint64, uint64>>;

  hb_buffer_t*                               _hb_buf{};
  std::vector<ImageHandle>                   _glyph_atlas;
  Font*                                      _notdef_font{};
  std::vector<std::unique_ptr<Font>>         _fonts;
  FontStyleMap<uint>                         _font_idxs;
  StyleFontIdxs                              _style_font_idxs;
  FontStyleMap<TextIDTextHashPairs>          _cached_texts_with_missing_glyphs;
  ParseResultMap                             _cached_text_parse_results;
  TextMap<TextParseResultHandle>             _last_ready_text_parse_results;
  ParseResultPool                            _parse_result_pool;
  FontStyleMap<std::unordered_set<uint>>     _missing_glyphs;
  FallbackFontIdxMap                         _fallback_font_idxs;
  std::vector<TextParseResultHandle>         _discard_text_parse_result_handles;
  std::unordered_set<TextParseResultHandle>  _generating_results;
  FontStyleMap<GlyphKeySet>                  _pending_reload_missing_glyphs;
  GlyphKeyMap<GlyphInfo>                     _glyph_infos;
  GlyphKeySet                                _uncached_glyphs;
  GlyphKeySet                                _ungenerated_glyphs;
  PendingCopyGlyphsInfoType                  _pending_copy_glyphs;
  PendingCopyGlyphsInfoType                  _pending_copy_preload_glyphs;
  float                                      _max_ascender{};
  float                                      _max_height{};
  std::vector<Task<std::vector<MSDFBitmap>>> _generate_bitmap_tasks;
  SkylinePacker                              _packer;
)

using TextParseResultHandle = TextEngine::TextParseResultHandle;

}
