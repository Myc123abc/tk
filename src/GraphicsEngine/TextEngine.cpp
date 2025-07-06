#include "tk/GraphicsEngine/TextEngine.hpp"
#include "tk/ErrorHandling.hpp"
#include "tk/util.hpp"
#include "tk/log.hpp"

#include <thread>
#include <fstream>

namespace
{

////////////////////////////////////////////////////////////////////////////////
//                            Binary File
////////////////////////////////////////////////////////////////////////////////

struct alignas(4) Header
{
  char     magic[5] = "MSDF";
  uint32_t width{};
  uint32_t height{};
  uint32_t channel{};
};

void write_msdf_file(msdfgen::BitmapConstRef<float, 4> bitmap, std::string_view filename)
{
  Header header;
  header.width   = bitmap.width;
  header.height  = bitmap.height;
  header.channel = 4;

  std::ofstream out(filename.data(), std::ios::binary);
  out.write(reinterpret_cast<char const*>(&header), sizeof(header));
  out.write(reinterpret_cast<char const*>(bitmap.pixels), bitmap.width * bitmap.height * 4 * sizeof(float));
}

auto read_msdf_file(std::string_view filename)
{
  tk::graphics_engine::Bitmap bitmap;

  std::ifstream in(filename.data(), std::ios::binary);
  tk::throw_if(!in.is_open(), "failed to open {}", filename);

  Header header;
  in.read(reinterpret_cast<char*>(&header), sizeof(header));
  tk::throw_if(strcmp(header.magic, "MSDF"), "{} is not a msdf file", filename);

  bitmap.width  = header.width;
  bitmap.height = header.height;

  bitmap.data.resize(header.width * header.height * header.channel  );
  in.read(reinterpret_cast<char*>(bitmap.data.data()), bitmap.data.size() * sizeof(float));

  return bitmap;
}

// from imgui_draw.cpp
// 0x0020, 0x00FF Basic Latin + Latin Supplement
auto get_char_set()
{
  auto beg = 0x0020;
  auto end = 0x00ff;

  msdf_atlas::Charset char_set;
  for (; beg <= end; ++beg)
    char_set.add(beg);
  return char_set;
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
// init freetype
  _ft = msdfgen::initializeFreetype();
  throw_if(!_ft, "failed to init freetype");
}

TextEngine::~TextEngine()
{
  deinitializeFreetype(_ft);
}

auto TextEngine::load_font(std::filesystem::path const& path) -> Bitmap
{
  // load font
  auto font = loadFont(_ft, path.string().c_str());
  throw_if(!font, "failed to load font {}", path.string());

  // load glyphs and font geometry
  _font_geometry = msdf_atlas::FontGeometry(&_glyphs);
  _font_geometry.loadCharset(font, 1.0, msdf_atlas::Charset::ASCII);

  // pack
  auto packer = get_packer();
  throw_if(packer.pack(_glyphs.data(), _glyphs.size()), "failed to pack glyphs");

  Bitmap result;
  auto msdf_file = std::format("resources/{}.msdf", path.stem().string());
  if (std::filesystem::exists(msdf_file))
    result = read_msdf_file(msdf_file);
  else
  {
    int width, height;
    packer.getDimensions(width, height);

    // TODO: use msdf_atlas::Workload for msdfgen::edgeColoringByDistance
    // edge coloring
    for (auto& glyph : _glyphs)
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
    generator.generate(_glyphs.data(), _glyphs.size());
    
    // generate bitmap
    msdfgen::BitmapConstRef<float, 4> bitmap = generator.atlasStorage();

    result.data.resize(bitmap.width * bitmap.height * 4);
    memcpy(result.data.data(), bitmap.pixels, result.data.size() * sizeof(float));
    result.width  = bitmap.width;
    result.height = bitmap.height;

    write_msdf_file(bitmap, msdf_file.c_str());
  }

  _atlas_extent = { result.width, result.height };

  // destroy font
  destroyFont(font);

  return result;
}

auto TextEngine::parse_text(std::string_view text, glm::vec2 const& pos, float size, std::vector<Vertex>& vertices, std::vector<uint16_t>& indices) -> std::pair<glm::vec2, glm::vec2>
{
  glm::vec2 text_min, text_max;

  auto metrics  = _font_geometry.getMetrics();
  auto move     = glm::vec2(0, metrics.ascenderY);
  auto position = pos;
  auto u32str   = util::to_u32string(text);
  auto idx      = indices.empty() ? 0 : indices.back() + 1;

  static auto invalid_ch = '?';

  for (auto i = 0; i < u32str.size(); ++i, idx += 4)
  {
    auto ch    = static_cast<uint32_t>(u32str[i]);
    auto glyph = _font_geometry.getGlyph(ch);
    if (glyph == nullptr)
    {
      ch    = invalid_ch;
      glyph = _font_geometry.getGlyph(invalid_ch);
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

    if (i == 0)
      text_min = min;
    if (i == u32str.size() - 1)
      text_max = max;

    if (i < u32str.size() - 1)
    {
      double advance;
      auto next_ch    = static_cast<uint32_t>(u32str[i + 1]);
      auto next_glyph = _font_geometry.getGlyph(next_ch);
      if (next_glyph == nullptr)
        next_ch = invalid_ch;
      throw_if(_font_geometry.getAdvance(advance, ch, next_ch) == false,
               "failed to get advance between {} and {}", ch, next_ch);
      position.x += advance * size;
    }

    al /= _atlas_extent.x;
    ab /= _atlas_extent.y;
    ar /= _atlas_extent.x;
    at /= _atlas_extent.y;

    vertices.append_range(std::vector<Vertex>
    {
      { min,              { al, at } },
      { { max.x, min.y }, { ar, at } },
      { { min.x, max.y }, { al, ab } },
      { max,              { ar, ab } },
    });

    indices.append_range(std::vector<uint16_t>
    {
      static_cast<uint16_t>(idx + 0), static_cast<uint16_t>(idx + 1), static_cast<uint16_t>(idx + 2),
      static_cast<uint16_t>(idx + 2), static_cast<uint16_t>(idx + 1), static_cast<uint16_t>(idx + 3),
    });
  }

  return { text_min, text_max };
}

}}