#pragma once

#include "ui/ui.hpp"
#include "util/base.hpp"
#include "config.hpp"
#include "../renderer/resource/image_manager.hpp"
#include "../util/double_buffer.hpp"

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
  void init(std::string_view path) noexcept;
  void destroy() const noexcept;

  auto name()  const noexcept { return _name;  }
  auto style() const noexcept { return _style; }

  auto find_glyph(uint unicode) noexcept
  {
    return FT_Get_Char_Index(_face, unicode);
  }

  auto generate_sdf_bitmap(uint glyph_idx, uint unicode, FontStyle style) const noexcept -> SDFBitmap;

private:
  std::string _name;
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

  // auto get_vertices(float2 pos, float size, uint offset, float ascender, uint glyph_atlases_index) const noexcept -> std::vector<Vertex>
  // {
  //   // TODO: add vertical draw
  //   auto scale = get_scale(size);
  //   auto p0 = pos + pos_offset * scale;
  //   p0.y += ascender * scale;
  //   auto p1 = float2{ p0.x + extent.x * scale, p0.y };
  //   auto p2 = float2{ p0.x, p0.y + extent.y * scale };
  //   auto p3 = float2{ p1.x, p2.y };      
  //   return
  //   {
  //     { p0, { min_x, min_y }, offset, glyph_atlases_index },
  //     { p1, { max_x, min_y }, offset, glyph_atlases_index },
  //     { p2, { min_x, max_y }, offset, glyph_atlases_index },
  //     { p3, { max_x, max_y }, offset, glyph_atlases_index },
  //   };
  // }

  // static auto get_indices(uint16& index) noexcept -> std::vector<uint16>
  // {
  //   auto idx = index;
  //   index += 4;
  //   return
  //   {
  //     static_cast<uint16>(idx + 0), static_cast<uint16>(idx + 1), static_cast<uint16>(idx + 2),
  //     static_cast<uint16>(idx + 2), static_cast<uint16>(idx + 1), static_cast<uint16>(idx + 3),
  //   };
  // }

  static auto get_next_position(float2 pos, float size, float2 advance) noexcept
  {
    return pos + advance * get_scale(size);
  }
};

Singleton(TextEngine, g_text_engine,
  friend class Font;
public:
  void init() noexcept;
  void destroy() const noexcept;

  void load_font(std::string_view path) noexcept;

  struct ParseResult
  {
    std::vector<float2> advances;
    float               max_ascender{};
    float               max_height{};
    std::u32string      text;
  };
  auto parse(std::string_view text, FontStyle style) noexcept -> ParseResult;

  void upload_uncached_glyphs() noexcept;

  auto& access_swaped_pending_copy_glyphs() noexcept { return _pending_copy_glyphs.access(); }

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
  FontStyleMap<TextMap<ParseResult>>               _cached_text_advances;
  FontStyleMap<std::unordered_set<uint>>           _missing_glyphs;
  FontStyleMap<UnicodeMap<GlyphInfo>>              _glyph_infos;
  FontStyleMap<UnicodeMap<std::pair<Font*, uint>>> _uncached_glyphs;
  
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

}
