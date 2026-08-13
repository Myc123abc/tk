#pragma once

#include "../../util/object_pool.hpp"
#include "../../renderer/resource/image_manager.hpp"
#include "../../util/thread_pool.hpp"
#include "font.hpp"
#include "glyph.hpp"
#include "packer.hpp"

namespace tk::ui {

using PendingCopyGlyphsInfoType = std::unordered_map<uint, std::vector<std::pair<MSDFBitmap, float2>>>;

Singleton(TextEngine, g_text_engine,
  friend class Font;
  friend class GlyphInfo;
public:
  void init() noexcept;
  void destroy() noexcept;

  auto load_font(std::string_view path) noexcept -> std::expected<FontInfo, FontLoadErrorType>;

  struct ParseResult
  {
    std::vector<float2>   advances;
    std::vector<float2>   offsets;
    float2                extent;
    float                 ascender{};
    bool                  is_vertical{};
    std::vector<GlyphKey> glyph_info_keys;
    GlyphKeySet           generating_glyph_info_keys;

    ParseResult() noexcept = default;
  };
  using ParseResultPool       = ObjectPool<ParseResult>;
  using TextParseResultHandle = ParseResultPool::Handle;
  auto parse(std::string_view text, FontStyle style, std::string_view family, TextDirection direction) noexcept -> TextParseResultHandle;
  auto& get_parse_result(TextParseResultHandle handle) const noexcept { return _parse_result_pool[handle]; }

  void update() noexcept;

  void clear_pending_copy_glyphs() noexcept;
  auto const& get_glyph_info(GlyphKey const& key) const noexcept { return _glyph_infos.at(key); }

  void postprocess() noexcept;

private:
  struct ParseKey
  {
    size_t        text_hash{};
    size_t        family_hash{};
    FontStyle     style{};
    TextDirection direction{};

    auto operator==(ParseKey const&) const noexcept -> bool = default;
  };

  struct ParseKeyHash
  {
    auto operator()(ParseKey const& key) const noexcept
    {
      return generic_hash(key.text_hash, key.style, key.family_hash, key.direction);
    }
  };

private:
  auto split_text(std::u32string_view text, FontStyleKey key) noexcept -> std::vector<std::pair<std::u32string_view, Font*>>;
  auto find_font(uint unicode, FontStyleKey key) noexcept -> Font*;
  auto find_notdef_glyph_font() noexcept -> Font*;
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
  using TextMap            = std::unordered_map<size_t, T>;
  using ParseResultMap     = std::unordered_map<ParseKey, TextParseResultHandle, ParseKeyHash>;
  using StyleFontIdxs      = std::array<std::vector<uint>, static_cast<size_t>(FontStyle::italic_bold) + 1>;
  using FallbackFontIdxMap = FontStyleMap<std::unordered_map<uint, uint>>;

  FT_Library                                 _ft{};
  hb_buffer_t*                               _hb_buf{};
  std::vector<ImageHandle>                   _glyph_atlas;
  Font*                                      _notdef_font{};
  std::vector<std::unique_ptr<Font>>         _fonts;
  FontStyleMap<uint>                         _font_idxs;
  StyleFontIdxs                              _style_font_idxs;
  FontStyleMap<std::vector<ParseKey>>        _cached_texts_with_missing_glyphs;
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
