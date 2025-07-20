#include "tk/GraphicsEngine/TextEngine/TextEngine.hpp"
#include "tk/ErrorHandling.hpp"
#include "tk/GraphicsEngine/GraphicsEngine.hpp"
#include "tk/util.hpp"


/*
TODO:
  add multiple fonts, move load font to example.cpp. (use charset overlap percent to choose charset for per font)
  dynamic load unloaded glyph, update atlas image descriptor for sdf fragment to location uv to right atlas
*/

namespace tk { namespace graphics_engine {

TextEngine::TextEngine(GraphicsEngine* engine)
  : _engine(engine)
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

auto TextEngine::parse_text(std::string_view text, glm::vec2 const& pos, float size, bool italic, std::vector<Vertex>& vertices, std::vector<uint16_t>& indices, uint32_t offset, uint16_t& idx) -> std::pair<glm::vec2, glm::vec2>
{
  // FIXME: tmp
  auto& font = _fonts.back();

  glm::vec2 text_min{ FLT_MAX }, text_max{};

  auto move     = glm::vec2(0, font.metrics.ascenderY);
  auto position = pos;
  auto scale    = size / Font::Font_Size;

  auto info = process_text(text);
  for (auto i = 0; i < info.text.size(); ++i, idx += 4)
  {
    auto const glyph = font.glyphs.at(info.text[i]);

    auto min = glm::vec2(glyph.pl, -glyph.pt);
    auto max = glm::vec2(glyph.pr, -glyph.pb);
    min += move;
    max += move;
    min *= size;
    max *= size;
    min += position;
    max += position;

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
      { p0, { glyph.al, glyph.at }, offset },
      { p1, { glyph.ar, glyph.at }, offset },
      { p2, { glyph.al, glyph.ab }, offset },
      { p3, { glyph.ar, glyph.ab }, offset },
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

struct Box
{
  double         rect_w{};
  double         rect_h{};
  msdfgen::Range range{};
  double         scale{};
  double         translate_x{};
  double         translate_y{};
};

auto tryPack(Font const& font, msdfgen::Shape::Bounds const& bounds, msdfgen::Shape const& shape, double scale) -> Box
{
  Box box;
  box.scale = scale * font.geo_scale;
  box.range = msdfgen::Range{ 2. / scale } / font.geo_scale;;
  double l = bounds.l, b = bounds.b, r = bounds.r, t = bounds.t;
  l += box.range.lower, b += box.range.lower;
  r -= box.range.lower, t -= box.range.lower;
  shape.boundMiters(l, b, r, t, -box.range.lower, 1, 1);
  double w = box.scale*(r-l);
  box.rect_w = (int) ceil(w)+1;
  box.translate_x = -l+.5*(box.rect_w-w)/box.scale;
  int sb = (int) floor(box.scale*b-.5);
  int st = (int) ceil(box.scale*t+.5);
  box.rect_h = st-sb;
  box.translate_y = -sb/box.scale;
  return box;
}

/*
 * 1. find which font can handle which characters
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
    std::vector<msdfgen::Bitmap<float, 4>> bitmaps;
    auto altas_extent = GraphicsEngine::get_atlas_extent();

    auto& info = res.first->second;
    for (auto i = 0; i < text.size();)
    {
      auto pair = util::to_utf32(text.data() + i);

      // TODO: 1.
      auto& font = _fonts.back();

      // check whether glyph is cached
      if (font.glyphs.find(pair.first) == font.glyphs.end())
      {
        // not cached, generate glyph msdf bitmap
        msdfgen::Shape shape;
        if (msdfgen::loadGlyph(shape, font.handle, pair.first, msdfgen::FONT_SCALING_NONE) && shape.validate())
        {
          if (shape.contours.empty())
          {
            // cache glyph
            font.glyphs.emplace(pair.first, Font::Glyph{});
            // add glyph to text
            info.text += pair.first;
            continue;
          }

          shape.normalize();
          shape.inverseYAxis = true;
          msdfgen::edgeColoringInkTrap(shape, 3.0);

          // get bounds of glyph and normalize extent
          auto bounds = shape.getBounds();
          //glm::vec2 extent{ bounds.r - bounds.l, bounds.t - bounds.b };

          // tryPack
          auto box = tryPack(font, bounds, shape, Font::Font_Size);

          //extent *= font.scale;
          //extent.x = floor(extent.x);
          //extent.y = floor(extent.y);

          // store and generate bitmap
          bitmaps.emplace_back(box.rect_w, box.rect_h);
          msdfgen::generateMTSDF(bitmaps.back(), shape, box.range, box.scale, { box.translate_x, box.translate_y });
          
          // get and normalize atlas coordinate
          auto glyph_pos = _engine->get_glyph_pos({ box.rect_w, box.rect_h });
          //glm::vec4 atlas_coord = { glyph_pos.x, glyph_pos.y + extent.y, glyph_pos.x + extent.x, glyph_pos.y };
          glm::vec4 atlas_coord{};
          atlas_coord.x = glyph_pos.x + .5;
          atlas_coord.y = glyph_pos.y + box.rect_h - .5;
          atlas_coord.z = glyph_pos.x + box.rect_w - .5;
          atlas_coord.w = glyph_pos.y + .5;
          //atlas_coord.x += .5;
          //atlas_coord.y -= .5;
          //atlas_coord.z -= .5;
          //atlas_coord.w += .5;
          atlas_coord.x /= altas_extent.x;
          atlas_coord.y /= altas_extent.y;
          atlas_coord.z /= altas_extent.x;
          atlas_coord.w /= altas_extent.y;
          // normalize bounds
          double invBoxScale = 1/box.scale;
          bounds.l = font.geo_scale*(-box.translate_x+(+.5)*invBoxScale);
          bounds.b = font.geo_scale*(-box.translate_y+(+.5)*invBoxScale);
          bounds.r = font.geo_scale*(-box.translate_x+(+box.rect_w-.5)*invBoxScale);
          bounds.t = font.geo_scale*(-box.translate_y+(+box.rect_h-.5)*invBoxScale);

          // cache glyph
          font.glyphs.emplace(pair.first, Font::Glyph{ atlas_coord.x, atlas_coord.y, atlas_coord.z, atlas_coord.w, bounds.l, bounds.b, bounds.r, bounds.t });

          // add glyph to text
          info.text += pair.first;
        }
        else
        {
          // TODO:
          // glyphs not exist in this font, find next font
          // if all font not have this glyph, use invalid symbol glyph

          info.text += '?'; // TODO: use my unique invalid symbol avoid font not have ? glyph
        }
      }
      else
        info.text += pair.first;

      i += pair.second;
    }

    // upload unloaded glyphs
    _engine->upload_glyph(bitmaps);

    // it should be some subtext for different font
    auto& font = _fonts.back();
    hb_buffer_reset(_hb_buffer);
    hb_buffer_add_utf32(_hb_buffer, reinterpret_cast<uint32_t*>(info.text.data()), info.text.size(), 0, -1);
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