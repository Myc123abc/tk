#include "tk/GraphicsEngine/TextEngine/TextEngine.hpp"
#include "tk/ErrorHandling.hpp"
#include "tk/log.hpp"

#include <thread>
#include <fstream>
#include <span>


// maybe i need delete msdf-atlas-gen.h, can avoid redefinition
//#include <msdfgen/core/ShapeDistanceFinder.hpp>

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

void Font::destroy() const noexcept
{
  msdfgen::destroyFont(handle);
  FT_Done_Face(face);
  hb_font_destroy(hb_font);
}

auto Font::init(FT_Library ft, std::filesystem::path const& path) -> Bitmap
{
  name = path.string();

  load_font(ft);

  load_metrics();
  
  // TODO: only load my charset, not msdf_atlas::CharSet
  //       and should be loaded by highest coverage of different charsets
  loaded_charset = get_latin_charset().second;
  load_charset();

  geometry = msdf_atlas::FontGeometry(&glyph_geos);
  auto packer = get_packer();

  Bitmap result;
  auto msdf_file = std::format("resources/{}.msdf", path.stem().string());

  // load cached file if have
  if (std::filesystem::exists(msdf_file))
  {
    auto res = read_msdf_file(msdf_file);
    result         = res.first;
    loaded_charset = res.second;
    geometry.loadCharset(handle, 1.0, get_msdf_charset(loaded_charset));
    // HACK: I don't know why must pack then geometry can load glyph_geos
    //       if continue use msdf-gen-atlas, dynamic load also must call packer.pack
    //       but I will replace to use msdfgen first
    throw_if(packer.pack(glyph_geos.data(), glyph_geos.size()), "failed to pack glyph_geos");
  }
  // otherwise generate atlas and cache file
  else
  {
    // TODO: change to choose best charset for per font
    auto charsets = get_latin_charset();
    geometry.loadCharset(handle, 1.0, charsets.first);
    loaded_charset = charsets.second;

    // pack
    throw_if(packer.pack(glyph_geos.data(), glyph_geos.size()), "failed to pack glyph_geos");

    int width, height;
    packer.getDimensions(width, height);

    // TODO: use msdf_atlas::Workload for msdfgen::edgeColoringByDistance
    // edge coloring
    for (auto& glyph : glyph_geos)
      glyph.edgeColoring(msdfgen::edgeColoringInkTrap, 3.0, 0);

    // set attribute of generator
    msdf_atlas::GeneratorAttributes attr;
    attr.config.overlapSupport = true;
    attr.scanlinePass          = true;

    // config generator
    msdf_atlas::ImmediateAtlasGenerator<float, 4, msdf_atlas::mtsdfGenerator, msdf_atlas::BitmapAtlasStorage<float, 4>> generator(width, height);
    generator.setAttributes(attr);
    generator.setThreadCount(std::thread::hardware_concurrency() / 2);
    
    log::info("generate msdf font atlas of {}", name);
    generator.generate(glyph_geos.data(), glyph_geos.size());
    
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

void Font::load_font(FT_Library ft)
{
  throw_if(FT_New_Face(ft, name.c_str(), 0, &face),
           "failed to load font {}", name);
  throw_if(FT_Set_Pixel_Sizes(face, 0, Font_Size),
           "failed to set em size of {}", name);
  handle = msdfgen::adoptFreetypeFont(face);
  throw_if(!handle, "failed to load font {}", name);
  hb_font = hb_ft_font_create(face, nullptr);
}

void Font::load_metrics()
{
  throw_if(!msdfgen::getFontMetrics(metrics, handle, msdfgen::FONT_SCALING_NONE), 
           "failed to load metrics of {}", name);
  if (metrics.emSize <= 0)
  {
    log::warn("emSize of {} is {}, default set to {}", name, metrics.emSize, Default_Font_Units_Per_EM);
    metrics.emSize = Default_Font_Units_Per_EM;
  }

  _scale = 1.0 / metrics.emSize;

  metrics.emSize             *= _scale;
  metrics.ascenderY          *= _scale;
  metrics.descenderY         *= _scale;
  metrics.lineHeight         *= _scale;
  metrics.underlineY         *= _scale;
  metrics.underlineThickness *= _scale;
}

// INFO: If want to load all charset in font, reference https://github.com/Haeri/elementalDraw/blob/26bd8e7664745542f55b9b6b2af5b58c76e642b6/src/font.cpp
void Font::load_charset()
{
  uint32_t total_size{};
  for (auto const& pair : loaded_charset)
  {
    total_size += pair.second - pair.first + 1;
  }
  glyphs.reserve(total_size);

  for (auto const& pair : loaded_charset)
  {
    for (auto i = pair.first; i <= pair.second; ++i)
    {
      // check glyph existence
      msdfgen::GlyphIndex index;
      if (msdfgen::getGlyphIndex(index, handle, i) == false)
        continue;

      // load glyph
      Glyph glyph;
      throw_if(!msdfgen::loadGlyph(glyph.shape, handle, index, msdfgen::FONT_SCALING_NONE, &glyph.advance) &&
               glyph.shape.validate(),
               "failed to load glyph (unicode : {})", i);
      glyph.codepoint = i;
      glyph.index     = index.getIndex();
      glyph.advance  *= _scale;
      glyph.shape.normalize();
      glyph.bounds    = glyph.shape.getBounds();

      // Determine if shape is winded incorrectly and reverse it in that case
      //msdfgen::Point2 outerPoint(glyph.bounds.l-(glyph.bounds.r-glyph.bounds.l)-1, glyph.bounds.b-(glyph.bounds.t-glyph.bounds.b)-1);
      //if (msdfgen::SimpleTrueShapeDistanceFinder::oneShotDistance(glyph.shape, outerPoint) > 0) {
      //  for (msdfgen::Contour &contour : glyph.shape.contours)
      //    contour.reverse();
      //}

      glyphs.emplace_back(glyph);
    }
  }
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

}}