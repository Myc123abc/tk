#pragma once

/*
TODO:
  1. bold, iteral, outline
  2. split text to different lanuage
  3. move pos from baseline to left-top corner
  4. get line height
  5. use vertical layout
  6. use multiple atlas  
*/

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

namespace tk { namespace graphics_engine {

  struct SDFBitmap
  {
    std::vector<uint8_t> data;
    glm::vec2            extent{};
    uint32_t             unicode{ std::numeric_limits<uint32_t>::max() };
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

  // TODO: static func get_line_height will get current line's fonts' max heighest line-height
  // TODO: use baseline-pos convert to left-top pos also need current line's max ascender of all of fonts
  struct GlyphInfo;
  class Font;
  class TextEngine
  {
  public:
    static constexpr auto Glyph_Atlas_Width  = 1024;
    static constexpr auto Glyph_Atlas_Height = Glyph_Atlas_Width;

    void init(MemoryAllocator& alloc);
    void destroy();

    void preload_glyphs(Command const& cmd);
    void calculate_write_position(glm::vec2 const& extent);
    void upload_glyphs(Command const& cmd, std::span<SDFBitmap> bitmaps);
    void upload_glyph(Command const& cmd, uint32_t unicode, uint8_t const* data, glm::vec2 extent, float left_offset, float up_offset);

    void load_font(std::string_view path);
    
    auto get_glyph_atlas_pointer() noexcept { return &_glyph_atlas; }

    auto has_uncached_glyphs(std::u32string_view text) -> bool;
    auto get_cached_glyph_info(uint32_t unicode) -> GlyphInfo*;
    void generate_sdf_bitmaps(Command const& cmd);

    auto find_glyph(uint32_t unicode) -> std::optional<std::pair<Font, uint32_t>>;

    auto calculate_advances(std::string_view text) -> std::vector<glm::vec2>;

  private:
    FT_Library                              _ft;
    std::vector<Font>                      _fonts;
    Image                                   _glyph_atlas; // TODO: expand to multiple, and should it have a limitation? and replace old one if attach limitation.
    Buffer                                  _glyph_atlas_buffer; // TODO: use common buffer which in graphics engine
    glm::vec2                               _current_write_position{};
    std::vector<glm::vec2>                  _write_positions{};
    float                                   _current_line_max_glyph_height{};
    std::unordered_map<uint32_t, GlyphInfo> _glyph_infos;
    std::unordered_map<uint32_t, std::pair<Font, uint32_t>> _wait_generate_sdf_bitmap_glyphs{};

    hb_buffer_t*                            _hb_buffer{};
    std::unordered_map<std::string, std::vector<glm::vec2>> _cached_text_advances;
    std::unordered_set<uint32_t>            _missing_glyphs;
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

    auto find_glyph(uint32_t unicode) -> uint32_t;
    auto generate_sdf_bitmap(uint32_t glyph_index, uint32_t unicode) -> SDFBitmap;

  private:
    std::string _name;
    FT_Face    _face;
    hb_font_t* _hb_font{};
  };

  struct GlyphInfo
  {
    // TODO: add field represent in which atlas
    float     min_x{};
    float     min_y{};
    float     max_x{};
    float     max_y{};
    glm::vec2 extent{};
    float     left_offset{};
    float     up_offset{};

    GlyphInfo(glm::vec2 pos, glm::vec2 extent, float left_offset, float up_offset) noexcept
      : extent(extent), left_offset(left_offset), up_offset(up_offset)
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

    auto get_vertices(glm::vec2 pos, float size, uint32_t offset) const noexcept -> std::vector<Vertex>
    {
      // TODO: add vertical draw in future
      auto scale = get_scale(size);
      auto pos_min_x = pos.x + left_offset * scale;
      auto pos_min_y = pos.y + up_offset * scale;
      auto pos_max_x = pos_min_x + extent.x * scale;
      auto pos_max_y = pos_min_y + extent.y * scale;
      return
      {
        { { pos_min_x, pos_min_y }, { min_x, min_y }, offset },
        { { pos_max_x, pos_min_y }, { max_x, min_y }, offset },
        { { pos_min_x, pos_max_y }, { min_x, max_y }, offset },
        { { pos_max_x, pos_max_y }, { max_x, max_y }, offset },
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

    static auto get_next_position(glm::vec2 pos, float size, glm::vec2 advance) noexcept
    {
      return pos + advance * get_scale(size);
    }
  };
}}