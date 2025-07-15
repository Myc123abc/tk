#pragma once

#include <filesystem>
#include <vector>
#include <map>

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

struct Glyph
{
  uint32_t               codepoint{};
  uint32_t               index{}; // TODO: is it useful?
  msdfgen::Shape         shape;
  msdfgen::Shape::Bounds bounds;
  double                 advance{};
};

struct Font
{
  auto init(FT_Library ft, std::filesystem::path const& path) ->Bitmap;
  void destroy() const noexcept;

  auto contain(uint32_t glyph) -> bool;

  static constexpr auto Default_Font_Units_Per_EM{ 2048.0 };

private:
  void load_font(FT_Library ft);
  void load_metrics();
  void load_charset();

  double _scale{};
  std::map<std::pair<uint32_t, uint32_t>, double> _kernings;

public:
  std::string                                name;
  FT_Face                                    face{};
  msdfgen::FontHandle*                       handle{};
  msdfgen::FontMetrics                       metrics;
  std::vector<std::pair<uint32_t, uint32_t>> loaded_charset;
  std::vector<Glyph>                         glyphs; // TODO: should I use map?
  
  // FIXME: discard
  msdf_atlas::FontGeometry                   geometry;
  std::vector<msdf_atlas::GlyphGeometry>     glyph_geos;
  glm::vec2                                  atlas_extent{};
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
  FT_Library        _ft{};
  std::vector<Font> _fonts{};
};

}}