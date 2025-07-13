#pragma once

#include <filesystem>
#include <vector>

#include <glm/glm.hpp>

#include <ft2build.h>
#include FT_FREETYPE_H

#include <msdf-atlas-gen/msdf-atlas-gen.h>

namespace tk { namespace graphics_engine {

struct Bitmap
{
  std::vector<float> data;
  uint32_t           width{};
  uint32_t           height{};
};

struct Vertex;

struct Font
{
  auto init(FT_Library ft, std::filesystem::path const& path) ->Bitmap;
  void destroy() const noexcept;

  auto contain(uint32_t glyph) -> bool;

  FT_Face                                    face{};
  msdfgen::FontHandle*                       handle{};
  msdf_atlas::FontGeometry                   geometry;
  std::vector<msdf_atlas::GlyphGeometry>     glyphs;
  glm::vec2                                  atlas_extent{};
  std::vector<std::pair<uint32_t, uint32_t>> loaded_charset;
};

class TextEngine
{
public:
  TextEngine();
  ~TextEngine();

  auto load_font(std::filesystem::path const& path) -> Bitmap;

  auto parse_text(std::string_view text, glm::vec2 const& pos, float size, bool italic, std::vector<Vertex>& vertices, std::vector<uint16_t>& indices, uint32_t offset, uint16_t& idx) -> std::pair<glm::vec2, glm::vec2>;

  auto load_unloaded_glyph(uint32_t glyph) -> bool;

private:
  FT_Library _ft_library{};
  std::vector<Font> _fonts{};
};

}}