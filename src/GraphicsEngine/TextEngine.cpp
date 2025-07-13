#include "tk/GraphicsEngine/TextEngine.hpp"
#include "tk/ErrorHandling.hpp"
#include "tk/util.hpp"
#include "tk/log.hpp"
#include "tk/GraphicsEngine/GraphicsEngine.hpp"

#include <thread>
#include <fstream>

/*
TODO:
  add multiple fonts, move load font to example.cpp. (use charset overlap percent to choose charset for per font)
  remove msdge-gen-atlas, only use msdfgen to custom atlas generate
  dynamic load unloaded glyph, update atlas image descriptor for sdf fragment to location uv to right atlas
*/

namespace
{

////////////////////////////////////////////////////////////////////////////////
//                            Binary File
////////////////////////////////////////////////////////////////////////////////

// MSDF | width | height | channel | charset count | charset... | pixels...
struct alignas(4) Header
{
  char     magic[5] = "MSDF";
  uint32_t width{};
  uint32_t height{};
  uint32_t channel{};
};

void write_msdf_file(msdfgen::BitmapConstRef<float, 4> bitmap, std::string_view filename, std::span<std::pair<uint32_t, uint32_t>> charset)
{
  Header header;
  header.width   = bitmap.width;
  header.height  = bitmap.height;
  header.channel = 4;

  std::ofstream out(filename.data(), std::ios::binary);

  // write header
  out.write(reinterpret_cast<char const*>(&header), sizeof(header));

  // write charset
  auto count = charset.size();
  out.write(reinterpret_cast<char const*>(&count), sizeof(uint32_t));
  for (auto const& pair : charset)
  {
    out.write(reinterpret_cast<char const*>(&pair.first), sizeof(uint32_t));
    out.write(reinterpret_cast<char const*>(&pair.second), sizeof(uint32_t));
  }

  // write pixels
  out.write(reinterpret_cast<char const*>(bitmap.pixels), bitmap.width * bitmap.height * 4 * sizeof(float));
}

auto read_msdf_file(std::string_view filename)
{
  std::ifstream in(filename.data(), std::ios::binary);
  tk::throw_if(!in.is_open(), "failed to open {}", filename);

  // read header
  Header header;
  in.read(reinterpret_cast<char*>(&header), sizeof(header));
  tk::throw_if(strcmp(header.magic, "MSDF"), "{} is not a msdf file", filename);

  // read charset
  uint32_t count{};
  in.read(reinterpret_cast<char*>(&count), sizeof(uint32_t));
  std::vector<std::pair<uint32_t, uint32_t>> charset;
  charset.reserve(count);
  for (auto i = 0; i < count; ++i)
  {
    std::pair<uint32_t, uint32_t> pair;
    in.read(reinterpret_cast<char*>(&pair.first), sizeof(uint32_t));
    in.read(reinterpret_cast<char*>(&pair.second), sizeof(uint32_t));
    charset.emplace_back(pair);
  }

  // read pixels
  tk::graphics_engine::Bitmap bitmap;
  bitmap.width  = header.width;
  bitmap.height = header.height;
  bitmap.data.resize(header.width * header.height * header.channel);
  in.read(reinterpret_cast<char*>(bitmap.data.data()), bitmap.data.size() * sizeof(float));

  return std::pair{ bitmap, charset };
}

auto get_msdf_charset(std::span<std::pair<uint32_t, uint32_t>> charset)
{
  msdf_atlas::Charset msdf_charset;
  for (auto const& pair : charset)
    for (auto i = pair.first; i <= pair.second; ++i)
      msdf_charset.add(i);
  return msdf_charset;
}

// from imgui_draw.cpp
// 0x0020, 0x00FF Basic Latin + Latin Supplement
auto get_latin_charset()
{
  std::vector<std::pair<uint32_t, uint32_t>> charset
  {
    { 0x0020, 0x00ff },
  };
  return std::pair{ get_msdf_charset(charset), charset };
}

auto get_packer()
{
  msdf_atlas::TightAtlasPacker packer;
  packer.setDimensionsConstraint(msdf_atlas::DimensionsConstraint::MULTIPLE_OF_FOUR_SQUARE);
  packer.setSpacing(0);
  packer.setMinimumScale(32.0);
  packer.setPixelRange(2.0);
  packer.setUnitRange(0.0);
  packer.setMiterLimit(1.0);
  packer.setOriginPixelAlignment(false, true);
  packer.setInnerUnitPadding(0);
  packer.setOuterUnitPadding(0);
  packer.setInnerPixelPadding(0);
  packer.setOuterPixelPadding(0);
  return packer;
}

}

namespace tk { namespace graphics_engine {

TextEngine::TextEngine()
{
  throw_if(FT_Init_FreeType(&_ft_library), "failed to init freetype library");
}

void Font::destroy() const noexcept
{
  msdfgen::destroyFont(handle);
  FT_Done_Face(face);
}

TextEngine::~TextEngine()
{
  for (auto const& font : _fonts)
    font.destroy();
  FT_Done_FreeType(_ft_library);
}

auto TextEngine::load_font(std::filesystem::path const& path) -> Bitmap
{
  return _fonts.emplace_back(Font()).init(_ft_library, path);
}

auto Font::init(FT_Library ft, std::filesystem::path const& path) -> Bitmap
{
  // load font
  throw_if(FT_New_Face(ft, path.string().c_str(), 0, &face),
           "failed to load font {}", path.string());
  handle = msdfgen::adoptFreetypeFont(face);
  throw_if(!handle, "failed to load font {}", path.string());

  // load glyphs and font geometry
  geometry = msdf_atlas::FontGeometry(&glyphs);
  // TODO: change to choose best charset for per font
  auto charsets = get_latin_charset();
  geometry.loadCharset(handle, 1.0, charsets.first);
  loaded_charset = charsets.second;

  // pack
  auto packer = get_packer();
  throw_if(packer.pack(glyphs.data(), glyphs.size()), "failed to pack glyphs");

  Bitmap result;
  auto msdf_file = std::format("resources/{}.msdf", path.stem().string());

  // load cached file if have
  if (std::filesystem::exists(msdf_file))
  {
    auto res = read_msdf_file(msdf_file);
    result         = res.first;
    loaded_charset = res.second;
  }
  // otherwise generate atlas and cache file
  else
  {
    int width, height;
    packer.getDimensions(width, height);

    // TODO: use msdf_atlas::Workload for msdfgen::edgeColoringByDistance
    // edge coloring
    for (auto& glyph : glyphs)
      glyph.edgeColoring(msdfgen::edgeColoringInkTrap, 3.0, 0);

    // set attribute of generator
    msdf_atlas::GeneratorAttributes attr;
    attr.config.overlapSupport = true;
    attr.scanlinePass          = true;

    // config generator
    msdf_atlas::ImmediateAtlasGenerator<float, 4, msdf_atlas::mtsdfGenerator, msdf_atlas::BitmapAtlasStorage<float, 4>> generator(width, height);
    generator.setAttributes(attr);
    generator.setThreadCount(std::thread::hardware_concurrency() / 2);
    
    log::info("generate msdf font atlas of {}", path.string());
    generator.generate(glyphs.data(), glyphs.size());
    
    // generate bitmap
    msdfgen::BitmapConstRef<float, 4> bitmap = generator.atlasStorage();

    result.data.resize(bitmap.width * bitmap.height * 4);
    memcpy(result.data.data(), bitmap.pixels, result.data.size() * sizeof(float));
    result.width  = bitmap.width;
    result.height = bitmap.height;

    write_msdf_file(bitmap, msdf_file.c_str(), loaded_charset);
  }

  atlas_extent = { result.width, result.height };

  return result;
}

auto Font::contain(uint32_t glyph) -> bool
{
  for (auto const& pair : loaded_charset)
  {
    if (glyph >= pair.first && glyph <= pair.second)
      return true;
  }
  return false;
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

  auto metrics  = font.geometry.getMetrics();
  auto move     = glm::vec2(0, metrics.ascenderY);
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

  return { pos, { text_max.x, metrics.lineHeight * size + pos.y } };
}

}}