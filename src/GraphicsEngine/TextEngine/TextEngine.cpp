#include "tk/GraphicsEngine/TextEngine/TextEngine.hpp"
#include "tk/ErrorHandling.hpp"
#include "missing-glyph-sdf-bitmap.hpp"
#include "hb-ft.h"


#define check(x, msg) throw_if(x, "[TextEngine] {}", msg)

namespace tk { namespace graphics_engine {

////////////////////////////////////////////////////////////////////////////////
///                              Text Engine
////////////////////////////////////////////////////////////////////////////////

void TextEngine::init(MemoryAllocator& alloc)
{
  // initialize freetype
  check(FT_Init_FreeType(&_ft), "failed to initialize");
  
  // create glyph atlas and buffer
  _glyph_atlas = alloc.create_image(VK_FORMAT_R8_UNORM, Glyph_Atlas_Width, Glyph_Atlas_Height, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);
  _glyph_atlas_buffer = alloc.create_buffer(Glyph_Atlas_Width * Glyph_Atlas_Height, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT);

  // create harffbuzz buffer
  _hb_buffer = hb_buffer_create();
}

void TextEngine::destroy()
{
  hb_buffer_destroy(_hb_buffer);
  _glyph_atlas_buffer.destroy();
  _glyph_atlas.destroy();
  for (auto& font: _fonts)
    font.destory();
  check(FT_Done_FreeType(_ft), "failed to destroy");
}

void TextEngine::preload_glyphs(Command const& cmd)
{
  calculate_write_position({ Missing_Glyph_Width, Missing_Glyph_Height });
  upload_glyph(cmd, Missing_Glyph_Unicode, Missing_Glyph_SDF_Bitmap, { Missing_Glyph_Width, Missing_Glyph_Height }, Missing_Glyph_Left_Offset, Missing_Glyph_Up_Offset);
}

void TextEngine::calculate_write_position(glm::vec2 const& extent)
{
  check(extent.x >= Glyph_Atlas_Width || extent.y >= Glyph_Atlas_Height,
        "too big glyph sdf bitmap, cannot be stored in glyph atlas");
again:
  auto write_max_pos = _current_write_position + extent;
  
  if (write_max_pos.x < Glyph_Atlas_Width && write_max_pos.y < Glyph_Atlas_Height)
  {
    _write_positions.emplace_back(_current_write_position);
    _current_write_position.x = write_max_pos.x;
    _current_line_max_glyph_height = std::max(_current_line_max_glyph_height, extent.y);
    return;
  }
  else if (write_max_pos.x >= Glyph_Atlas_Width)
  {
    // move to next line
    _current_write_position.x = 0;
    _current_write_position.y += _current_line_max_glyph_height;
    _current_line_max_glyph_height = {};
    goto again;
  }
  else if (write_max_pos.y >= Glyph_Atlas_Height)
  {
    throw_if(true, "TODO: create new glyph atlas");
    _current_write_position.y = {};
    goto again;
  }
  assert(true);
}

void TextEngine::upload_glyph(Command const& cmd, uint32_t unicode, uint8_t const* data, glm::vec2 extent, float left_offset, float up_offset)
{
  // promise only one glyph to upload and not contain it
  assert(_write_positions.size() == 1 && !_glyph_infos.contains(unicode));
  // record glyph information
  GlyphInfo info(_write_positions[0], extent, left_offset, up_offset);

  // clear buffer
  _glyph_atlas_buffer.clear();
  // upload data to buffer
  _glyph_atlas_buffer.append(data, extent.x * extent.y);

  // copy glyph data from buffer to image
  copy(cmd, _glyph_atlas_buffer, 0, _glyph_atlas, _write_positions[0], extent);
  
  // set image layout for read
  _glyph_atlas.set_layout(cmd, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

  // clear position information
  _write_positions.clear();

  // store glyph information
  _glyph_infos.emplace(unicode, info);
}

void TextEngine::upload_glyphs(Command const& cmd, std::span<SDFBitmap> bitmaps)
{
  // consistence between bitmaps size and uv positions size
  assert(!bitmaps.empty() && bitmaps.size() == _write_positions.size());

  // clear buffer
  _glyph_atlas_buffer.clear();

  uint32_t buffer_offset{};
  uint32_t index{};
  for (auto const& bitmap : bitmaps)
  {
    // promise this is an uncached glyph
    assert(!_glyph_infos.contains(bitmap.unicode));

    auto byte_size = bitmap.extent.x * bitmap.extent.y;
    
    // get glyph information
    GlyphInfo info(_write_positions[index], bitmap.extent, bitmap.left_offset, bitmap.up_offset);
    // record glyph information
    _glyph_infos.emplace(bitmap.unicode, info);

    if (bitmap.valid())
    {
      // copy glyph data to buffer
      _glyph_atlas_buffer.append(bitmap.data.data(), byte_size);
      // copy glyph from buffer to image
      copy(cmd, _glyph_atlas_buffer, buffer_offset, _glyph_atlas, _write_positions[index], bitmap.extent);
    }

    // move to next one
    ++index;
    buffer_offset += byte_size;
  }

  // set image layout for read
  _glyph_atlas.set_layout(cmd, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

  // clear stored uvs
  _write_positions.clear();
}

void TextEngine::generate_sdf_bitmaps(Command const& cmd)
{
  // promise need to generate
  assert(!_wait_generate_sdf_bitmap_glyphs.empty());

  std::vector<SDFBitmap> bitmaps;
  bitmaps.reserve(_wait_generate_sdf_bitmap_glyphs.size());
  _write_positions.reserve(_wait_generate_sdf_bitmap_glyphs.size());

  for (auto& [unicode, pair] : _wait_generate_sdf_bitmap_glyphs)
  {
    auto& [font, glyph_index] = pair;
    // generate sdf bitmaps
    bitmaps.emplace_back(font.generate_sdf_bitmap(glyph_index, unicode));
    // calculate every bitmaps position in atlas
    calculate_write_position(bitmaps.back().extent);
  }

  // upload to atlas
  upload_glyphs(cmd, bitmaps);

  _wait_generate_sdf_bitmap_glyphs.clear();
}

void TextEngine::load_font(std::string_view path)
{
  for (auto const& font : _fonts)
    throw_if(font._name == path, "[TextEngine] {} is already exist", path);
  _fonts.emplace_back(Font::create(_ft, path));

  // TODO: if dynamic add new fonts, remove have missing glyphs' cached text and in next rendering reshaped by hurfbuzz
}

auto TextEngine::find_glyph(uint32_t unicode) -> std::optional<std::pair<Font, uint32_t>>
{
  // promise not generated
  assert(!_glyph_infos.contains(unicode));
  for (auto& font : _fonts)
  {
    auto glyph_index = font.find_glyph(unicode);
    if (glyph_index)
    {
      return std::make_pair(font, glyph_index);
    }
  }
  return {};
}

auto TextEngine::get_cached_glyph_info(uint32_t unicode) -> GlyphInfo*
{
  // promise cached
  assert(_glyph_infos.contains(unicode) || _missing_glyphs.contains(unicode));
  auto it = _glyph_infos.find(unicode);
  if (it != _glyph_infos.end())
    return &it->second;
  return &_glyph_infos.at(Missing_Glyph_Unicode);
}

auto TextEngine::has_uncached_glyphs(std::u32string_view text) -> bool
{
  for (auto const& unicode : text)
  {
    if (!_glyph_infos.contains(unicode) && !_missing_glyphs.contains(unicode) && !_wait_generate_sdf_bitmap_glyphs.contains(unicode))
    {
      auto res = find_glyph(unicode);
      if (res)
      {
        _wait_generate_sdf_bitmap_glyphs.emplace(unicode, res.value());
      }
      else
      {
        _missing_glyphs.emplace(unicode); // TODO: can use for dynamic load font refind missing glyph,
                                          // and reshape in cached text which have missing glyphs
      }
    }
  }
  return !_wait_generate_sdf_bitmap_glyphs.empty();
}

auto TextEngine::calculate_advances(std::string_view text) -> std::vector<glm::vec2>
{
  if (_cached_text_advances.contains(text.data())) return _cached_text_advances[text.data()];

  hb_buffer_reset(_hb_buffer);
  // TODO: promise text is same script and direction
  hb_buffer_add_utf8(_hb_buffer, text.data(), -1, 0, -1);
  // TODO: use icu or smt or else like suckless's one to set script, direction, language
  hb_buffer_guess_segment_properties(_hb_buffer);
  // TODO: select right font to shape
  hb_shape(_fonts[0]._hb_font, _hb_buffer, nullptr, 0);

  auto length          = hb_buffer_get_length(_hb_buffer);
  auto glyph_infos     = hb_buffer_get_glyph_infos(_hb_buffer, nullptr);
  auto glyph_positions = hb_buffer_get_glyph_positions(_hb_buffer, nullptr);
  std::vector<glm::vec2> advances;
  advances.reserve(length);
  for (auto i = 0; i < length; ++i)
    advances.emplace_back(static_cast<float>(glyph_positions[i].x_advance) / 64,
                          static_cast<float>(glyph_positions[i].y_advance) / 64);

  _cached_text_advances.emplace(text.data(), advances);
  return advances;
}

////////////////////////////////////////////////////////////////////////////////
///                                 Font
////////////////////////////////////////////////////////////////////////////////

auto Font::create(FT_Library ft, std::string_view path) -> Font
{
  Font font;
  font._name = path;
  check(FT_New_Face(ft, path.data(), 0, &font._face), "failed to load font");
  check(FT_Set_Pixel_Sizes(font._face, 0, Pixel_Size), "failed to set pixel size");
  font._hb_font = hb_ft_font_create(font._face, nullptr);
  return font;
}

void Font::destory()
{
  hb_font_destroy(_hb_font);
  check(FT_Done_Face(_face), "failed to destroy font");
}

auto Font::find_glyph(uint32_t unicode) -> uint32_t
{
  return FT_Get_Char_Index(_face, unicode);
}

auto Font::generate_sdf_bitmap(uint32_t glyph_index, uint32_t unicode) -> SDFBitmap
{
  assert(glyph_index != 0);
  check(FT_Load_Glyph(_face, glyph_index, FT_LOAD_RENDER), "failed to load glyph with render");
  check(FT_Render_Glyph(_face->glyph, FT_RENDER_MODE_SDF), "failed to render sdf bitmap");
  SDFBitmap bitmap;
  bitmap.extent      = { _face->glyph->bitmap.width, _face->glyph->bitmap.rows };
  bitmap.unicode     = unicode;
  bitmap.left_offset = _face->glyph->bitmap_left;
  bitmap.up_offset   = -_face->glyph->bitmap_top;
  bitmap.data.resize(bitmap.extent.x * bitmap.extent.y);
  memcpy(bitmap.data.data(), _face->glyph->bitmap.buffer, bitmap.data.size());
  return bitmap;
}

}}