#include "tk/GraphicsEngine/TextEngine.hpp"
#include "tk/ErrorHandling.hpp"
#include "tk/util.hpp"
#include "tk/GraphicsEngine/GraphicsEngine.hpp"


/*
TODO:
  add multiple fonts, move load font to example.cpp. (use charset overlap percent to choose charset for per font)
  dynamic load unloaded glyph, update atlas image descriptor for sdf fragment to location uv to right atlas
*/

namespace tk { namespace graphics_engine {

TextEngine::TextEngine()
{
  throw_if(FT_Init_FreeType(&_ft), "failed to init freetype library");
}

TextEngine::~TextEngine()
{
  for (auto const& font : _fonts)
    font.destroy();
  FT_Done_FreeType(_ft);
}

auto TextEngine::load_font(std::filesystem::path const& path) -> Bitmap
{
  return _fonts.emplace_back(Font()).init(_ft, path);
}

auto TextEngine::load_unloaded_glyph(uint32_t glyph) -> bool
{
  // FIXME: tmp
  auto& font = _fonts.back();
  msdfgen::Shape shape;
  if (FT_Get_Char_Index(font.face, glyph))
  {
    assert(false); // TODO: implement dynamic load glyph, also in multiple fonts
    if (!msdfgen::loadGlyph(shape, font.handle, glyph, msdfgen::FONT_SCALING_EM_NORMALIZED))
      return false;
    shape.normalize();
    msdfgen::edgeColoringSimple(shape, 3.0);
    msdfgen::Bitmap<float, 4> mtsdf(32, 32);
    msdfgen::SDFTransformation t(msdfgen::Projection(32.0, msdfgen::Vector2(0.125, 0.125)), msdfgen::Range(0.125));
    msdfgen::generateMTSDF(mtsdf, shape, t);
    return true;
  }
  return false;
}

auto TextEngine::parse_text(std::string_view text, glm::vec2 const& pos, float size, bool italic, std::vector<Vertex>& vertices, std::vector<uint16_t>& indices, uint32_t offset, uint16_t& idx) -> std::pair<glm::vec2, glm::vec2>
{
  // FIXME: tmp
  auto& font = _fonts.back();

  glm::vec2 text_min{ FLT_MAX }, text_max{};

  auto move     = glm::vec2(0, font.metrics.ascenderY);
  auto position = pos;
  auto u32str   = util::to_u32string(text);

  static auto invalid_ch = '?';

  for (auto i = 0; i < u32str.size(); ++i, idx += 4)
  {
    auto ch    = static_cast<uint32_t>(u32str[i]);
    auto glyph = font.geometry.getGlyph(ch);
    if (glyph == nullptr)
    {
      if (load_unloaded_glyph(ch)) // TODO: also do on next_ch
      {
        
      }
      else
      {
        ch    = invalid_ch;
        glyph = font.geometry.getGlyph(invalid_ch);
      }
    }

    double al, ab, ar, at;
    glyph->getQuadAtlasBounds(al, ab, ar, at);
    double pl, pb, pr, pt;
    glyph->getQuadPlaneBounds(pl, pb, pr, pt);

    auto min = glm::vec2(pl, -pt);
    auto max = glm::vec2(pr, -pb);
    min += move;
    max += move;
    min *= size;
    max *= size;
    min += position;
    max += position;

    if (i < u32str.size() - 1)
    {
      double advance;
      auto next_ch    = static_cast<uint32_t>(u32str[i + 1]);
      auto next_glyph = font.geometry.getGlyph(next_ch);
      if (next_glyph == nullptr)
        next_ch = invalid_ch;
      throw_if(font.geometry.getAdvance(advance, ch, next_ch) == false,
               "failed to get advance between {} and {}", ch, next_ch);
      position.x += advance * size;
    }

    al /= font.atlas_extent.x;
    ab /= font.atlas_extent.y;
    ar /= font.atlas_extent.x;
    at /= font.atlas_extent.y;

    auto p0 = min;
    auto p1 = glm::vec2{ max.x, min.y };
    auto p2 = glm::vec2{ min.x, max.y };
    auto p3 = max;

    auto italic_p0 = p0;
    auto italic_p1 = p1;
    auto italic_p2 = p2;
    auto italic_p3 = p3;

    static constexpr auto factor = 0.4;
    auto italic_offset = italic_p3.y * factor;
    italic_p0.x -= italic_p0.y * factor;
    italic_p1.x -= italic_p1.y * factor;
    italic_p2.x -= italic_p2.y * factor;
    italic_p0.x += italic_offset;
    italic_p1.x += italic_offset;
    italic_p2.x += italic_offset;

    //if (i == 0)
    //  text_min.x = italic_p2.x;
    if (i == u32str.size() - 1)
      text_max.x = italic_p1.x;
    //text_min.y = std::min(text_min.y, italic_p0.y);
    //text_max.y = std::max(text_max.y, italic_p2.y);

    if (italic)
    {
      p0 = italic_p0;
      p1 = italic_p1;
      p2 = italic_p2;
      p3 = italic_p3;
    }

    vertices.append_range(std::vector<Vertex>
    {
      { p0, { al, at }, offset },
      { p1, { ar, at }, offset },
      { p2, { al, ab }, offset },
      { p3, { ar, ab }, offset },
    });

    indices.append_range(std::vector<uint16_t>
    {
      static_cast<uint16_t>(idx + 0), static_cast<uint16_t>(idx + 1), static_cast<uint16_t>(idx + 2),
      static_cast<uint16_t>(idx + 2), static_cast<uint16_t>(idx + 1), static_cast<uint16_t>(idx + 3),
    });
  }

  return { pos, { text_max.x, font.metrics.lineHeight * size + pos.y } };
}

}}