#pragma once

#include <filesystem>
#include <vector>

#include <glm/glm.hpp>
#include <msdf-atlas-gen/msdf-atlas-gen.h>

namespace tk { namespace graphics_engine {

struct Bitmap
{
  std::vector<float> data;
  uint32_t           width{};
  uint32_t           height{};
};

struct Vertex
{
  glm::vec2 pos{};
  glm::vec2 uv{};
};

class TextEngine
{
public:
  TextEngine();
  ~TextEngine();

  auto load_font(std::filesystem::path const& path) -> Bitmap;

  auto parse_text(std::string_view text, glm::vec2 const& pos, float size, std::vector<Vertex>& vertices, std::vector<uint16_t>& indices) -> std::pair<glm::vec2, glm::vec2>;

private:
  msdfgen::FreetypeHandle* _ft{};

  // TODO: make vector load multiple fonts
  msdf_atlas::FontGeometry               _font_geometry;
  std::vector<msdf_atlas::GlyphGeometry> _glyphs;
  glm::vec2                              _atlas_extent{};
};

}}