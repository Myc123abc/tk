//
// text engine
// 
// use sdf render text, and bold, italic support by font styles.
// when some font style not load, will be display missing glyphs.
// use have responsibility to load all styles for font (italic, bold, italic bold)
// unless them never use styles they not load
//

#pragma once

#include <ft2build.h>
#include FT_FREETYPE_H
#include <glm/glm.hpp>
#include <hb.h>

#include <string_view>
#include <vector>
#include <span>
#include <unordered_map>
#include <unordered_set>
#include <optional>

#include "../MemoryAllocator.hpp"
#include "../types.hpp"
#include "tk/type.hpp"

namespace tk { namespace graphics_engine {

  struct SDFBitmap
  {
    std::vector<uint8_t> data;
    glm::vec2            extent{};
    uint32_t             unicode{ std::numeric_limits<uint32_t>::max() };
    type::FontStyle      style{};
    float                left_offset{};
    float                up_offset{};

    auto valid() const noexcept
    { 
      return unicode != std::numeric_limits<uint32_t>::max() &&
             !data.empty()                                   &&
             extent.x > 0                                    &&
             extent.y > 0;
    }
  };

  struct TextPosInfo
  {
    std::vector<glm::vec2> advances;
    float                  max_ascender{};
    float                  max_height{};
  };

  struct GlyphInfo;
  class Font;
  class TextEngine
  {
  public:
    static constexpr auto Glyph_Atlas_Width  = 2048;
    static constexpr auto Glyph_Atlas_Height = Glyph_Atlas_Width;

    void init(MemoryAllocator& alloc);
    void destroy();

    // return true, need to expand descriptors because of new glyph atlases be created
    auto frame_begin(Command const& cmd) -> bool;

    void preload_glyphs(Command const& cmd);
    void calculate_write_position(glm::vec2 const& extent);
    void upload_glyphs(std::span<SDFBitmap> bitmaps);
    void upload_glyph(Command const& cmd, uint32_t unicode, uint8_t const* data, glm::vec2 extent, float left_offset, float up_offset, type::FontStyle style);

    void load_font(std::string_view path);
    
    auto get_glyph_atlases() const noexcept { return _glyph_atlases; }

    auto has_uncached_glyphs(std::u32string_view text, type::FontStyle style) -> bool;
    auto get_cached_glyph_info(uint32_t unicode, type::FontStyle style) -> GlyphInfo*;
    void generate_sdf_bitmaps();

    auto find_glyph(uint32_t unicode, type::FontStyle style) -> std::optional<std::pair<std::reference_wrapper<Font>, uint32_t>>;
    
    auto calculate_text_pos_info(std::string_view text, type::FontStyle style) -> std::pair<TextPosInfo, std::u32string>;
    auto split_text_by_font(std::u32string_view text, type::FontStyle style) -> std::vector<std::pair<std::u32string_view, Font*>>;
    auto find_suitable_font(uint32_t unicode, type::FontStyle style) -> Font*;

    template <typename T>
    static auto has(T& map, uint32_t unicode, type::FontStyle style) noexcept
    {
      if (map.contains(style))
        return map[style].contains(unicode);
      return false;
    }
    auto glyph_infos_has(uint32_t unicode, type::FontStyle style) noexcept
    {
      return has(_glyph_infos, unicode, style);
    }
    auto missing_glyphs_has(uint32_t unicode, type::FontStyle style) noexcept
    {
      return has(_missing_glyphs, unicode, style);
    }
    auto wait_generate_glyphs_has(uint32_t unicode, type::FontStyle style) noexcept
    {
      return has(_wait_generate_sdf_bitmap_glyphs, unicode, style);
    }
    auto wait_generate_glyphs_size() const noexcept -> uint32_t;

  private:
    template <typename T>
    using FontStyleMap = std::unordered_map<type::FontStyle, T>;
    template <typename T>
    using UnicodeMap   = std::unordered_map<uint32_t, T>;
    template <typename T>
    using TextMap      = std::unordered_map<std::string, T>;

    FT_Library                                            _ft;
    FontStyleMap<std::vector<Font>>                       _fonts;
    MemoryAllocator*                                      _mem_alloc{};
    std::vector<Image>                                    _glyph_atlases;
    Buffer                                                _glyph_atlas_buffer;
    glm::vec2                                             _current_write_position{};
    std::vector<std::pair<uint32_t, glm::vec2>>           _write_positions{};
    float                                                 _current_line_max_glyph_height{};
    FontStyleMap<UnicodeMap<GlyphInfo>>                   _glyph_infos;
    FontStyleMap<UnicodeMap<std::pair<Font, uint32_t>>>   _wait_generate_sdf_bitmap_glyphs{};
    hb_buffer_t*                                          _hb_buffer{};
    FontStyleMap<TextMap<TextPosInfo>>                    _cached_text_advances;
    std::vector<std::pair<type::FontStyle, std::string>>  _cached_texts_with_missing_glyphs;
    FontStyleMap<std::unordered_set<uint32_t>>            _missing_glyphs;
    float                                                 _max_ascender{};
    float                                                 _max_height{};
    std::unordered_map<uint32_t, std::vector<CopyRegion>> _copy_regions;
    uint32_t                                              _current_glyph_atlas_index{};
    bool                                                  _new_glyph_atlas{};
  };

  class Font
  {
    friend class TextEngine;

  public:
    // TODO: small pixel size will lead complex glyph generate sdf bitmap not right when render in big size
    //       try use dynamic pixel size adjust by complexity of glyph
    //       and store in different glyph size altas to save memory
    static constexpr auto Pixel_Size = 32;

    static auto create(FT_Library ft, std::string_view path) -> Font;
    void destory();

    auto find_glyph(uint32_t unicode) noexcept -> uint32_t;
    auto generate_sdf_bitmap(uint32_t glyph_index, uint32_t unicode, type::FontStyle style) -> SDFBitmap;

  private:
    std::string     _name;
    FT_Face         _face;
    hb_font_t*      _hb_font{};
    type::FontStyle _style{};
    float           _ascender{};
    float           _height{};
  };

  struct GlyphInfo
  {
    uint32_t  glyph_atlas_index{};
    float     min_x{};
    float     min_y{};
    float     max_x{};
    float     max_y{};
    glm::vec2 extent{};
    glm::vec2 pos_offset{};

    GlyphInfo(uint32_t glyph_atlas_index, glm::vec2 pos, glm::vec2 extent, float left_offset, float up_offset) noexcept
      : glyph_atlas_index(glyph_atlas_index), extent(extent), pos_offset(left_offset, up_offset)
    {
      min_x = (pos.x + 0.5f) / TextEngine::Glyph_Atlas_Width;
      min_y = (pos.y + 0.5f) / TextEngine::Glyph_Atlas_Height;
      max_x = (pos.x + extent.x - 0.5f) / TextEngine::Glyph_Atlas_Width;
      max_y = (pos.y + extent.y - 0.5f) / TextEngine::Glyph_Atlas_Height;
    }

    static auto get_scale(float size) noexcept
    {
      return size / Font::Pixel_Size;
    }

    auto get_vertices(glm::vec2 const& pos, float size, uint32_t offset, float ascender, uint32_t glyph_atlases_index) const noexcept -> std::vector<Vertex>
    {
      // TODO: add vertical draw in future
      auto scale = get_scale(size);
      auto p0 = pos + pos_offset * scale;
      p0.y += ascender * scale;
      auto p1 = glm::vec2{ p0.x + extent.x * scale, p0.y };
      auto p2 = glm::vec2{ p0.x, p0.y + extent.y * scale };
      auto p3 = glm::vec2{ p1.x, p2.y };      
      return
      {
        { p0, { min_x, min_y }, offset, glyph_atlases_index },
        { p1, { max_x, min_y }, offset, glyph_atlases_index },
        { p2, { min_x, max_y }, offset, glyph_atlases_index },
        { p3, { max_x, max_y }, offset, glyph_atlases_index },
      };
    }

    static auto get_indices(uint16_t& index) noexcept -> std::vector<uint16_t>
    {
      auto idx = index;
      index += 4;
      return
      {
        static_cast<uint16_t>(idx + 0), static_cast<uint16_t>(idx + 1), static_cast<uint16_t>(idx + 2),
        static_cast<uint16_t>(idx + 2), static_cast<uint16_t>(idx + 1), static_cast<uint16_t>(idx + 3),
      };
    }

    static auto get_next_position(glm::vec2 const& pos, float size, glm::vec2 const& advance) noexcept
    {
      return pos + advance * get_scale(size);
    }
  };
}}