#pragma once

#include "ui/ui.hpp"
#include "util/base.hpp"
#include "../config.hpp"
#include "../../renderer/resource/image_manager.hpp"
#include "../../util/double_buffer.hpp"
#include "../../util/object_pool.hpp"
#include "../../renderer/resource/shader_type.hpp"

#include <ft2build.h>
#include FT_FREETYPE_H
#include <hb.h>

#include <string>
#include <unordered_set>
#include <unordered_map>
#include <memory_resource>

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

struct SDFBitmap
{
  std::pmr::vector<uint8> data;
  uint2                   extent{};
  uint                    unicode{ std::numeric_limits<uint>::max() };
  FontStyle               style{};
  float                   left_offset{};
  float                   up_offset{};

  auto to_bitmap_view() const noexcept -> renderer::BitmapView
  {
    return { data.data(), extent.x, extent.y, extent.x };
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

  auto generate_sdf_bitmap(uint glyph_idx, uint unicode, FontStyle style) const noexcept -> SDFBitmap;

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

  GlyphInfo(uint glyph_atlas_index, float2 pos, float2 extent, float left_offset, float up_offset) noexcept
    : glyph_atlas_index(glyph_atlas_index), extent(extent), pos_offset(left_offset, up_offset)
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
    std::vector<float2> advances;
    float2              extent;
    float               ascender{};
    std::u32string      text;
    FontStyle           style;

    ParseResult() noexcept = default;
  };
  using ParseResultPool       = ObjectPool<ParseResult>;
  using TextParseResultHandle = ParseResultPool::Handle;
  auto parse(std::string_view text, FontStyle style, std::string_view family) noexcept -> TextParseResultHandle;
  auto& get_parse_result(TextParseResultHandle handle) const noexcept { return _parse_result_pool[handle]; }

  void upload_uncached_glyphs() noexcept;

  auto& access_swaped_pending_copy_glyphs() noexcept { return _pending_copy_glyphs.access(); }

  auto const& get_glyph_infos(FontStyle style) noexcept { return _glyph_infos[style]; }
  auto get_missing_glyph_info() noexcept -> GlyphInfo const&;

  void clear_discard_text_parse_results() noexcept;

private:
  auto split_text(std::u32string_view text, FontStyle style) noexcept -> std::vector<std::pair<std::u32string_view, Font*>>;
  auto find_font(uint unicode, FontStyle style) noexcept -> Font*;
  void add_uncached_glyphs(std::u32string_view text, FontStyle style) noexcept;
  auto find_glyph(uint unicode, FontStyle style) noexcept -> std::optional<std::pair<Font*, uint>>;

  template <MapType Map>
  static auto has(Map& map, uint unicode, FontStyle style) noexcept { return map.contains(style) ? map[style].contains(unicode) : false; }
  auto glyph_infos_has(uint unicode, FontStyle style)     noexcept { return has(_glyph_infos, unicode, style);     }
  auto missing_glyphs_has(uint unicode, FontStyle style)  noexcept { return has(_missing_glyphs, unicode, style);  }
  auto uncached_glyphs_has(uint unicode, FontStyle style) noexcept { return has(_uncached_glyphs, unicode, style); }

  auto calc_glyph_pos(float2 extent) noexcept -> std::pair<uint, float2>;

private:
  template <typename T>
  using FontStyleMap = std::unordered_map<FontStyle, T>;
  template <typename T>
  using UnicodeMap   = std::unordered_map<uint, T>;
  template <typename T>
  using TextMap      = std::unordered_map<std::string, T>;

  FT_Library   _ft{};
  hb_buffer_t* _hb_buf{};

  std::vector<ImageHandle>                         _glyph_atlas;
  FontStyleMap<std::vector<Font>>                  _fonts;
  std::vector<std::pair<FontStyle, std::string>>   _cached_texts_with_missing_glyphs;
  FontStyleMap<TextMap<TextParseResultHandle>>     _cached_text_parse_results;
  ParseResultPool                                  _parse_result_pool;
  FontStyleMap<std::unordered_set<uint>>           _missing_glyphs;
  FontStyleMap<UnicodeMap<GlyphInfo>>              _glyph_infos;
  FontStyleMap<UnicodeMap<std::pair<Font*, uint>>> _uncached_glyphs;
  std::vector<TextParseResultHandle>               _discard_text_parse_result_handles;
  
  using PendingCopyGlyphsInfoType = std::unordered_map<uint, std::vector<std::pair<SDFBitmap, float2>>>;
  DoubleBuffer<PendingCopyGlyphsInfoType> _pending_copy_glyphs;
  
  float _max_ascender{};
  float _max_height{};

  struct MemmoryPool
  {
  private:
    std::pmr::monotonic_buffer_resource _pool;

  public:
    template <typename T>
    auto vector() noexcept { return std::pmr::vector<T>{ &_pool }; }
  } _mem_pool;
)

using TextParseResultHandle = TextEngine::TextParseResultHandle;

}
