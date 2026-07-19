#pragma once

#include "ui/ui.hpp"
#include "util/base.hpp"
#include "../config.hpp"
#include "../../renderer/resource/image_manager.hpp"
#include "../../util/object_pool.hpp"
#include "../../renderer/resource/shader_type.hpp"
#include "../../util/thread_pool.hpp"

#include <msdfgen.h>
#include <ft2build.h>
#include FT_FREETYPE_H
#include <hb.h>

#include <string>
#include <unordered_set>
#include <unordered_map>

namespace tk::ui {

template<typename T>
concept MapType =
requires(T t)
{
  typename T::key_type;
  typename T::mapped_type;

  t.find(typename T::key_type{});
  t.begin();
  t.end();
};

struct GlyphKey
{
  friend struct GlyphKeyHash;

  GlyphKey() noexcept = default;
  GlyphKey(FontStyle style, uint unicode) noexcept : _k(static_cast<uint64>(style) << 32 | unicode) {}

  auto has(uint unicode) const noexcept -> bool { return _k & 0xffffffff; }

  auto operator==(GlyphKey const&) const noexcept -> bool = default;

// private:
  uint64 _k{};
};

struct GlyphKeyHash
{
  auto operator()(GlyphKey k) const noexcept
  {
    return std::hash<uint64>{}(k._k);
  }
};

struct MSDFBitmap
{
  std::vector<uint8> data;
  uint2              extent{};
  float2             pos_offset{};
  GlyphKey           glyph_key;

  auto to_bitmap_view() const noexcept -> renderer::BitmapView
  {
    return { data.data(), extent.x, extent.y, extent.x * 4 };
  }

  auto empty() const noexcept { return !extent.x || !extent.y; }
};

class TextEngine;
class Font
{
  friend class TextEngine;
public:
  auto init(std::string_view path) noexcept -> uint8;
  void destroy() const noexcept;

  auto name()  const noexcept { return _name;  }
  auto style() const noexcept { return _style; }

  auto find_glyph(uint unicode) const noexcept
  {
    return FT_Get_Char_Index(_face, unicode);
  }

  auto get_glyph_advance(uint glyph_idx) const noexcept -> float2;
  auto generate_msdf_bitmap(uint glyph_idx, GlyphKey key) const noexcept -> MSDFBitmap;

private:
  std::string _name;
  std::string _family;
  FontStyle   _style;
  FT_Face     _face{};
  hb_font_t*  _hb_font{};
  float       _ascender{};
  float       _height{};
};

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
    float2                extent;
    float                 ascender{};
    std::vector<GlyphKey> glyph_info_keys;
    std::unordered_set<GlyphKey, GlyphKeyHash> generating_glyph_info_keys;

    ParseResult() noexcept = default;
  };
  using ParseResultPool       = ObjectPool<ParseResult>;
  using TextParseResultHandle = ParseResultPool::Handle;
  auto parse(std::string_view text, FontStyle style, std::string_view family) noexcept -> TextParseResultHandle;
  auto& get_parse_result(TextParseResultHandle handle) const noexcept { return _parse_result_pool[handle]; }

  void submit_bitmap_generation_tasks() noexcept;
  void upload_bitmaps() noexcept;

  void clear_pending_copy_glyphs() noexcept { _pending_copy_glyphs.clear(); }
  auto const& get_glyph_info(GlyphKey key) const noexcept { return _glyph_infos.at(key); }
  auto get_notdef_glyph_info(FontStyle style) noexcept -> GlyphInfo const&;

  void postprocess() noexcept;

private:
  auto split_text(std::u32string_view text, FontStyle style) noexcept -> std::vector<std::pair<std::u32string_view, Font*>>;
  auto find_font(uint unicode, FontStyle style) noexcept -> Font*;
  auto find_notdef_glyph_font(FontStyle style) noexcept -> std::pair<Font*, bool>;
  void add_uncached_glyphs(std::u32string_view text, FontStyle style, ParseResult& result) noexcept;
  auto add_notdef_glyph(FontStyle style) noexcept -> bool;
  auto find_glyph(uint unicode, FontStyle style) noexcept -> std::optional<std::pair<Font*, uint>>;
  auto calc_glyph_pos(float2 extent) noexcept -> std::pair<uint, float2>;

private:
  template <typename T>
  using FontStyleMap = std::unordered_map<FontStyle, T>;
  template <typename T>
  using TextMap      = std::unordered_map<size_t, T>;

  FT_Library   _ft{};
  hb_buffer_t* _hb_buf{};

  std::vector<ImageHandle>                         _glyph_atlas;
  FontStyleMap<std::vector<Font>>                  _fonts;
  FontStyleMap<std::vector<size_t>>                _cached_texts_with_missing_glyphs;
  FontStyleMap<TextMap<TextParseResultHandle>>     _cached_text_parse_results;
  ParseResultPool                                  _parse_result_pool;
  FontStyleMap<std::unordered_set<uint>>           _missing_glyphs;
  std::vector<TextParseResultHandle>               _discard_text_parse_result_handles;
  std::unordered_set<FontStyle>                    _missing_notdef_font_styles;
  std::unordered_set<TextParseResultHandle>        _generating_results;

  std::unordered_map<GlyphKey, GlyphInfo, GlyphKeyHash>              _glyph_infos;
  std::unordered_set<GlyphKey, GlyphKeyHash>                         _uncached_glyphs;
  std::unordered_map<GlyphKey, std::pair<Font*, uint>, GlyphKeyHash> _ungenerated_glyphs;
  
  using PendingCopyGlyphsInfoType = std::unordered_map<uint, std::vector<std::pair<MSDFBitmap, float2>>>;
  PendingCopyGlyphsInfoType _pending_copy_glyphs;
  
  float _max_ascender{};
  float _max_height{};

  std::vector<Task<std::vector<MSDFBitmap>>> _generate_bitmap_tasks;
)

using TextParseResultHandle = TextEngine::TextParseResultHandle;

}
