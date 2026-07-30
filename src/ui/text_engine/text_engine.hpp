#pragma once

#include "../config.hpp"
#include "../../util/object_pool.hpp"
#include "../../renderer/resource/shader_type.hpp"
#include "../../renderer/resource/image_manager.hpp"
#include "../../util/thread_pool.hpp"
#include "font.hpp"

namespace tk::ui {

struct GlyphInfo
{
  uint   glyph_atlas_index{};
  float  min_x{};
  float  min_y{};
  float  max_x{};
  float  max_y{};
  float2 extent{};
  float2 pos_offset{};

  GlyphInfo(uint glyph_atlas_index, float2 pos, float2 extent, float2 pos_offset) noexcept
    : glyph_atlas_index(glyph_atlas_index), extent(extent), pos_offset(pos_offset)
  {
    min_x = (pos.x + 0.5f) / Glyph_Atlas_Width;
    min_y = (pos.y + 0.5f) / Glyph_Atlas_Height;
    max_x = (pos.x + extent.x - 0.5f) / Glyph_Atlas_Width;
    max_y = (pos.y + extent.y - 0.5f) / Glyph_Atlas_Height;
  }

  static auto get_scale(float size) noexcept
  {
    return size / FT_Pixel_Size;
  }

  void set_vertices(renderer::Vertex* vtx, float2 pos, float size, float ascender, Color color, Color outer_color, float outer_width) const noexcept;

  static auto get_next_position(float2 pos, float size, float2 advance) noexcept
  {
    return pos + advance * get_scale(size);
  }
};

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
    std::vector<GlyphKey> glyph_info_keys;
    GlyphKeySet           generating_glyph_info_keys;

    ParseResult() noexcept = default;
  };
  using ParseResultPool       = ObjectPool<ParseResult>;
  using TextParseResultHandle = ParseResultPool::Handle;
  auto parse(std::string_view text, FontStyle style, std::string_view family) noexcept -> TextParseResultHandle;
  auto& get_parse_result(TextParseResultHandle handle) const noexcept { return _parse_result_pool[handle]; }

  void update() noexcept;

  void clear_pending_copy_glyphs() noexcept { _pending_copy_glyphs.clear(); }
  auto const& get_glyph_info(GlyphKey key) const noexcept { return _glyph_infos.at(key); }

  void postprocess() noexcept;

private:
  struct ParseKey
  {
    size_t      text_hash{};
    size_t      family_hash{};
    FontStyle   style{};

    auto operator==(ParseKey const&) const noexcept -> bool = default;
  };

  struct ParseKeyHash
  {
    auto operator()(ParseKey const& key) const noexcept
    {
      return generic_hash(key.text_hash, key.style, key.family_hash);
    }
  };

private:
  auto split_text(std::u32string_view text, FontStyleKey key) noexcept -> std::vector<std::pair<std::u32string_view, Font*>>;
  auto find_font(uint unicode, FontStyleKey key) noexcept -> Font*;
  auto find_notdef_glyph_font() noexcept -> Font*;
  void add_uncached_glyph(Font* font, uint glyph_idx, GlyphKey key, ParseResult& result) noexcept;
  auto add_notdef_glyph() noexcept -> bool;
  auto calc_glyph_pos(float2 extent) noexcept -> std::pair<uint, float2>;
  void regenerate_missing_glyphs(Font* font, FontStyleKey key) noexcept;
  void remove_missing_glyphs(FontStyleKey key) noexcept;
  void reload_missing_glyphs() noexcept;

  void submit_bitmap_generation_tasks() noexcept;
  void upload_bitmaps() noexcept;

private:
  template <typename T>
  using TextMap                   = std::unordered_map<size_t, T>;
  using ParseResultMap            = std::unordered_map<ParseKey, TextParseResultHandle, ParseKeyHash>;
  using PendingCopyGlyphsInfoType = std::unordered_map<uint, std::vector<std::pair<MSDFBitmap, float2>>>;
  using StyleFontIdxs             = std::array<std::vector<uint>, static_cast<size_t>(FontStyle::italic_bold) + 1>;
  using FallbackFontIdxMap        = FontStyleMap<std::unordered_map<uint, uint>>;

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
  float                                      _max_ascender{};
  float                                      _max_height{};
  std::vector<Task<std::vector<MSDFBitmap>>> _generate_bitmap_tasks;
)

using TextParseResultHandle = TextEngine::TextParseResultHandle;

}
