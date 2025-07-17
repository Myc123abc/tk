#include "tk/GraphicsEngine/TextEngine/TextEngine.hpp"
#include "tk/ErrorHandling.hpp"
#include "tk/GraphicsEngine/GraphicsEngine.hpp"


/*
TODO:
  add multiple fonts, move load font to example.cpp. (use charset overlap percent to choose charset for per font)
  dynamic load unloaded glyph, update atlas image descriptor for sdf fragment to location uv to right atlas
*/

namespace
{

auto to_unicode(std::string_view str) -> std::pair<uint32_t, uint32_t>
{
  assert(!str.empty());
  uint8_t ch = str[0];
  if (ch < 0x80)
    return { ch, 1 };
  else if ((ch & 0xE0) == 0xC0)
  {
    assert(str.size() > 1);
    return { (ch & 0x1F) << 6 | (str[1] & 0x3F), 2 };
  }
  else if ((ch & 0xF0) == 0xE0)
  {
    assert(str.size() > 2);
    return { (ch & 0x0F) << 12 | (str[1] & 0x3F) << 6 | (str[2] & 0x3F), 3 };
  }
  else if ((ch & 0xF8) == 0xF0)
  {
    assert(str.size() > 3);
    return { (str[0] & 0x07) << 18 | (str[1] & 0x3F) << 12 | (str[2] & 0x3F) << 6 | (str[3] & 0x3F), 4 };
  }
  assert(true);
  return {};
}

}

namespace tk { namespace graphics_engine {

TextEngine::TextEngine()
{
  throw_if(FT_Init_FreeType(&_ft), "failed to init freetype library");
  _hb_buffer = hb_buffer_create();
}

TextEngine::~TextEngine()
{
  hb_buffer_destroy(_hb_buffer);
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
  auto scale    = size / Font::Font_Size;
  static auto invalid_ch = '?';

  auto info = process_text(text);
  for (auto i = 0; i < info.text.size(); ++i, idx += 4)
  {
    auto glyph = font.geometry.getGlyph(info.text[i]);
    if (glyph == nullptr)
      glyph = font.geometry.getGlyph(invalid_ch);

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
    if (i == info.text.size() - 1)
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

    position += info.advances[i] * scale;
  }

  return { pos, { text_max.x, font.metrics.lineHeight * size + pos.y } };
}

/*
 * 1. find which font can handle which characters
 * 2. if the character can be handle with the font, but not be loaded,
 *    load it and store in dynamic atlas
 * 3. if the character not in any charset of fonts, replace it to '?'
 */
auto TextEngine::process_text(std::string_view text) -> TextInfo
{
  // cache raw str with text info
  // text info has replaced text
  // and other text info
  auto res = _cached_texts.try_emplace(std::string(text));
  if (res.second == false)
  {
    // TODO: return text info
  }
  else
  { 
    // replace unloaded glyph to ?
    // change text to unicode text
    // and iterate every unicode
    // if not have charset of font contain it
    // replace to ?
    auto& info = res.first->second;
    for (auto i = 0; i < text.size();)
    {
      auto pair = to_unicode(text.data() + i);

      //auto script = hb_unicode_script(hb_unicode_funcs_get_default(), pair.first);

      // TODO: 1.
      auto& font = _fonts.back();

      // TODO: 2.
      // FIXME: discard msdf-atlas-gen

      // get string replaced invalied glyph
      if (font.geometry.getGlyph(pair.first) == nullptr)
        info.text += '?';
      else
        info.text += pair.first;

      i += pair.second;
    }

    // it should be some subtext for different font
    auto& font = _fonts.back();
    hb_buffer_reset(_hb_buffer);
    hb_buffer_add_utf8(_hb_buffer, info.text.data(), -1, 0, -1);
    hb_buffer_guess_segment_properties(_hb_buffer);
    hb_shape(font.hb_font, _hb_buffer, nullptr, 0);

    auto length      = hb_buffer_get_length(_hb_buffer);
    auto glyph_infos = hb_buffer_get_glyph_infos(_hb_buffer, nullptr);
    auto positions   = hb_buffer_get_glyph_positions(_hb_buffer, nullptr);
    info.advances.reserve(length);
    for (auto i = 0; i < length; ++i)
      info.advances.emplace_back(positions[i].x_advance / 64, positions->y_advance / 64.);
  }
  return res.first->second;
}

}}