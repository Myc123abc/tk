#pragma once

#include <filesystem>
#include <vector>
#include <unordered_map>

#include <glm/glm.hpp>

#include <hb-ft.h>
#include <msdf-atlas-gen/msdf-atlas-gen.h>

namespace tk { namespace graphics_engine {

struct Bitmap
{
  std::vector<float> data;
  uint32_t           width{};
  uint32_t           height{};
};

class Font
{
public:
  auto init(FT_Library ft, std::filesystem::path const& path) ->Bitmap;
  void destroy() const noexcept;

  inline static constexpr auto Font_Size{ 32 };

private:
  inline static constexpr auto Default_Font_Units_Per_EM{ 2048.0 };

  void load_font(FT_Library ft);
  void load_metrics();
  auto get_charset() -> msdf_atlas::Charset;
  void load_glyphs();

  auto generate_atlas_cache(std::string_view filename) -> Bitmap;
  void write_cache_file(msdfgen::BitmapConstRef<float, 4> bitmap, std::string_view filename);
  auto read_cache_file(std::string_view filename) -> Bitmap;

public:
  std::string          name;
  FT_Face              face{};
  msdfgen::FontHandle* handle{};
  msdfgen::FontMetrics metrics;
  hb_font_t*           hb_font{};

  struct alignas(8) Glyph
  {
    double al{}, ab{}, ar{}, at{};
    double pl{}, pb{}, pr{}, pt{};
  };
  std::unordered_map<char32_t, Glyph> glyphs;
};

struct Vertex;
class TextEngine
{
public:
  TextEngine();
  ~TextEngine();

  auto load_font(std::filesystem::path const& path) -> Bitmap;

  auto parse_text(std::string_view text, glm::vec2 const& pos, float size, bool italic, std::vector<Vertex>& vertices, std::vector<uint16_t>& indices, uint32_t offset, uint16_t& idx) -> std::pair<glm::vec2, glm::vec2>;

  auto load_unloaded_glyph(uint32_t glyph) -> bool;

  inline auto empty() const noexcept { return _fonts.empty(); }

private:
  struct TextInfo;
  auto process_text(std::string_view text) -> TextInfo;

  struct TextInfo
  {
    std::u32string         text;
    std::vector<glm::vec2> advances;
  };
  std::unordered_map<std::string, TextInfo> _cached_texts;

private:
  FT_Library        _ft{};
  std::vector<Font> _fonts{};
  hb_buffer_t*      _hb_buffer{};
};

}}