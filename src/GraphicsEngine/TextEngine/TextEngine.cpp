#include "tk/GraphicsEngine/TextEngine/TextEngine.hpp"
#include "tk/ErrorHandling.hpp"
#include "missing-glyph-sdf-bitmap.hpp"
#include "tk/util.hpp"

#include <hb-ft.h>
#include <hb-ot.h>

#include <ranges>


#define check(x, msg) throw_if(x, "[TextEngine] {}", msg)

namespace tk { namespace graphics_engine {

////////////////////////////////////////////////////////////////////////////////
///                              Text Engine
////////////////////////////////////////////////////////////////////////////////

void TextEngine::init(MemoryAllocator& alloc)
{
  // initialize freetype
  check(FT_Init_FreeType(&_ft), "failed to initialize");
  
  _mem_alloc = &alloc;

  // create glyph atlas and buffer
  _glyph_atlases.emplace_back(alloc.create_image(VK_FORMAT_R8_UNORM, Glyph_Atlas_Width, Glyph_Atlas_Height, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT));
  // TODO: dynamic buffer
  _glyph_atlas_buffer = alloc.create_buffer(Glyph_Atlas_Width * Glyph_Atlas_Height, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT);

  // create harffbuzz buffer
  _hb_buffer = hb_buffer_create();
}

void TextEngine::destroy()
{
  hb_buffer_destroy(_hb_buffer);
  _glyph_atlas_buffer.destroy();
  for (auto& image : _glyph_atlases)
    image.destroy();
  for (auto& [_, fonts]: _fonts)
    for (auto& font : fonts)
      font.destory();
  check(FT_Done_FreeType(_ft), "failed to destroy");
}

void TextEngine::preload_glyphs(Command const& cmd)
{
  calculate_write_position({ Missing_Glyph_Width, Missing_Glyph_Height });
  upload_glyph(cmd, Missing_Glyph_Unicode, Missing_Glyph_SDF_Bitmap,
    { Missing_Glyph_Width, Missing_Glyph_Height },
    Missing_Glyph_Left_Offset, Missing_Glyph_Up_Offset,
    type::FontStyle::regular);
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
    // TODO: _glyph_atlases_indexs.emplace_back(_glyph_atlases.size() - 1);
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
    // TODO: update descriptor buffer
    _glyph_atlases.emplace_back(_mem_alloc->create_image(VK_FORMAT_R8_UNORM, Glyph_Atlas_Width, Glyph_Atlas_Height, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT));
    _current_write_position.y = 0;
    goto again;
  }
  assert(true);
}

void TextEngine::upload_glyph(Command const& cmd, uint32_t unicode, uint8_t const* data, glm::vec2 extent, float left_offset, float up_offset, type::FontStyle style)
{
  // promise only one glyph to upload and not contain it
  assert(_write_positions.size() == 1 && !glyph_infos_has(unicode, style));
  // record glyph information
  GlyphInfo info(_write_positions[0], extent, left_offset, up_offset);

  // clear buffer
  _glyph_atlas_buffer.clear();
  // upload data to buffer
  _glyph_atlas_buffer.append(data, extent.x * extent.y);

  // TODO:
  // copy glyph data from buffer to image
  copy(cmd, _glyph_atlas_buffer, 0, *get_first_glyph_atlas_pointer(), _write_positions[0], extent);
  
  // TODO:
  // set image layout for read
  get_first_glyph_atlas_pointer()->set_layout(cmd, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

  // clear position information
  _write_positions.clear();

  // store glyph information
  _glyph_infos[style].emplace(unicode, info);
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
    assert(!glyph_infos_has(bitmap.unicode, bitmap.style));

    auto byte_size = bitmap.extent.x * bitmap.extent.y;
    
    // get glyph information
    GlyphInfo info(_write_positions[index], bitmap.extent, bitmap.left_offset, bitmap.up_offset);
    // record glyph information
    _glyph_infos[bitmap.style].emplace(bitmap.unicode, info);

    if (bitmap.valid())
    {
      // copy glyph data to buffer
      _glyph_atlas_buffer.append(bitmap.data.data(), byte_size);
      // TODO:
      // copy glyph from buffer to image
      copy(cmd, _glyph_atlas_buffer, buffer_offset, *get_first_glyph_atlas_pointer(), _write_positions[index], bitmap.extent);
    }

    // move to next one
    ++index;
    buffer_offset += byte_size;
  }

  // TODO:
  // set image layout for read
  get_first_glyph_atlas_pointer()->set_layout(cmd, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

  // clear stored uvs
  _write_positions.clear();
}

void TextEngine::generate_sdf_bitmaps(Command const& cmd)
{
  // promise need to generate
  assert(!_wait_generate_sdf_bitmap_glyphs.empty());

  std::vector<SDFBitmap> bitmaps;
  bitmaps.reserve(wait_generate_glyphs_size());
  _write_positions.reserve(bitmaps.size());

  for (auto& [style, unicode_pairs] : _wait_generate_sdf_bitmap_glyphs)
  {
    for (auto& [unicode, pair] : unicode_pairs)
    {
      auto& [font, glyph_index] = pair;
      // generate sdf bitmaps
      bitmaps.emplace_back(font.generate_sdf_bitmap(glyph_index, unicode, style));
      // calculate every bitmaps position in atlas
      calculate_write_position(bitmaps.back().extent);
    }
  }

  // upload to atlas
  upload_glyphs(cmd, bitmaps);

  _wait_generate_sdf_bitmap_glyphs.clear();
}

void TextEngine::load_font(std::string_view path)
{
  for (auto const& [_, fonts] : _fonts)
    for (auto const& font : fonts)
      throw_if(font._name == path, "[TextEngine] {} is already exist", path);
  
  auto font = Font::create(_ft, path);
  _fonts[font._style].emplace_back(font);

  // clear missing glyphs and cached text advances
  _missing_glyphs.clear();
  for (auto const& [style, text] : _cached_texts_with_missing_glyphs)
    _cached_text_advances[style].erase(text);
  _cached_texts_with_missing_glyphs.clear();
}

auto TextEngine::find_glyph(uint32_t unicode, type::FontStyle style) -> std::optional<std::pair<std::reference_wrapper<Font>, uint32_t>>
{
  // promise not generated
  assert(!glyph_infos_has(unicode, style));
  for (auto& font : _fonts[style])
  {
    auto glyph_index = font.find_glyph(unicode);
    if (glyph_index)
    {
      return std::make_pair(std::ref(font), glyph_index);
    }
  }
  return {};
}

auto TextEngine::get_cached_glyph_info(uint32_t unicode, type::FontStyle style) -> GlyphInfo*
{
  // promise cached
  assert(glyph_infos_has(unicode, style) || missing_glyphs_has(unicode, style));
  auto& infos = _glyph_infos[style];
  if (auto it = infos.find(unicode); it != infos.end())
    return &it->second;
  return &_glyph_infos[type::FontStyle::regular].at(Missing_Glyph_Unicode);
}

auto TextEngine::has_uncached_glyphs(std::u32string_view text, type::FontStyle style) -> bool
{
  for (auto const& unicode : text)
  {
    if (!glyph_infos_has(unicode, style) && !missing_glyphs_has(unicode, style) && !wait_generate_glyphs_has(unicode, style))
    {
      if (auto res = find_glyph(unicode, style))
        _wait_generate_sdf_bitmap_glyphs[style].emplace(unicode, *res);
      else
        _missing_glyphs[style].emplace(unicode);
    }
  }
  return !_wait_generate_sdf_bitmap_glyphs[style].empty();
}

auto TextEngine::calculate_text_pos_info(std::string_view text, type::FontStyle style) -> std::pair<TextPosInfo, std::u32string>
{
  assert(!text.empty());

  auto u32str = util::to_u32string(text);

  // try to get cached text advances
  auto& cached_text_advances = _cached_text_advances[style];
  if (cached_text_advances.contains(text.data())) return { cached_text_advances[text.data()], u32str };

  // uncached, calculate advances
  std::vector<glm::vec2> advances;
  advances.reserve(u32str.size());
  bool has_missing_glyphs{};

  // split text by script
  for (auto const& [text, font] : split_text_by_font(u32str, style))
  {
    if (font)
    {
      hb_buffer_reset(_hb_buffer);
      hb_buffer_add_utf32(_hb_buffer, reinterpret_cast<uint32_t const*>(text.data()), text.size(), 0, -1);
      hb_buffer_guess_segment_properties(_hb_buffer);
      hb_shape(font->_hb_font, _hb_buffer, nullptr, 0);

      auto glyph_positions = hb_buffer_get_glyph_positions(_hb_buffer, nullptr);
      for (auto i = 0; i < text.size(); ++i)
        advances.emplace_back(static_cast<float>(glyph_positions[i].x_advance) / 64, static_cast<float>(glyph_positions[i].y_advance) / 64);
    }
    // if not have font, the text is missing glyphs
    else
    {
      has_missing_glyphs = true;
      static auto missing_glyph_position_info = glm::vec2{ Missing_Glyph_Advance_X * Missing_Glyph_Size / Font::Pixel_Size,
                                                           Missing_Glyph_Advance_Y * Missing_Glyph_Size / Font::Pixel_Size };
      advances.resize(advances.size() + text.size(), missing_glyph_position_info);
      continue;
    }
  }

  if (has_missing_glyphs) 
  {
    // cache text with missing glyphs
    _cached_texts_with_missing_glyphs.emplace_back(style, text.data());
    // update max info
    _max_ascender = std::max(_max_ascender, Missing_Glyph_Font_Ascender);
    _max_height   = std::max(_max_height, Missing_Glyph_Font_Height);
  }

  auto res = TextPosInfo{ advances, _max_ascender, _max_height };

  // cached calculate result
  cached_text_advances.emplace(text.data(), res);

  return { res, u32str };
}

auto TextEngine::split_text_by_font(std::u32string_view text, type::FontStyle style) -> std::vector<std::pair<std::u32string_view, Font*>>
{
  assert(!text.empty());

  std::vector<std::pair<std::u32string_view, Font*>> result;
  result.reserve(text.size());

  auto it_a      = text.begin();
  auto it_b      = it_a + 1;
  auto prev_font = find_suitable_font(*it_a, style);
  if (prev_font)
  {
    _max_ascender  = prev_font->_ascender;
    _max_height    = prev_font->_height;
  }

  while (it_b != text.end())
  {
    auto current_font = find_suitable_font(*it_b, style);
    if (current_font == prev_font)
    {
      ++it_b;
      continue;
    }
    else
    {
      result.emplace_back(std::u32string_view{ it_a, it_b }, prev_font);
      prev_font = current_font;
      it_a = it_b;
      ++it_b;

      if (prev_font)
      {
        _max_ascender = std::max(_max_ascender, prev_font->_ascender);
        _max_height   = std::max(_max_height, prev_font->_height);
      }
    }
  }

  // process last one
  assert(it_a < it_b);
  result.emplace_back(std::u32string_view{ it_a, it_b }, prev_font);

  return result;
}

auto TextEngine::find_suitable_font(uint32_t unicode, type::FontStyle style) -> Font*
{
  if (auto it = std::ranges::find_if(_fonts[style], [unicode](auto& font) { return font.find_glyph(unicode); });
      it != _fonts[style].end())
      return &*it;
  return {};
}

auto TextEngine::wait_generate_glyphs_size() const noexcept -> uint32_t
{
  uint32_t size{};
  for (auto const& [_, glyphs] : _wait_generate_sdf_bitmap_glyphs)
    size += glyphs.size();
  return size;
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

  font._ascender = static_cast<float>(font._face->ascender) * Pixel_Size / font._face->units_per_EM;
  font._height   = static_cast<float>(font._face->height)   * Pixel_Size / font._face->units_per_EM;

  using enum tk::type::FontStyle;
  switch (font._face->style_flags)
  {
  case FT_STYLE_FLAG_ITALIC:
    font._style = italic;
    break;

  case FT_STYLE_FLAG_BOLD:
    font._style = bold;
    break;

  case FT_STYLE_FLAG_ITALIC | FT_STYLE_FLAG_BOLD:
    font._style = italic_bold;
    break;
      
  default:
    font._style = regular;
    break;
  }

  return font;
}

void Font::destory()
{
  hb_font_destroy(_hb_font);
  check(FT_Done_Face(_face), "failed to destroy font");
}

auto Font::find_glyph(uint32_t unicode) noexcept -> uint32_t
{
  return FT_Get_Char_Index(_face, unicode);
}

auto Font::generate_sdf_bitmap(uint32_t glyph_index, uint32_t unicode, type::FontStyle style) -> SDFBitmap
{
  assert(glyph_index != 0);
  check(FT_Load_Glyph(_face, glyph_index, FT_LOAD_RENDER), "failed to load glyph with render");
  auto glyph = _face->glyph;
  check(FT_Render_Glyph(glyph, FT_RENDER_MODE_SDF), "failed to render sdf bitmap");
  auto ft_bitmap = _face->glyph->bitmap;
  SDFBitmap bitmap;
  bitmap.extent      = { ft_bitmap.width, ft_bitmap.rows };
  bitmap.unicode     = unicode;
  bitmap.style       = style;
  bitmap.left_offset = glyph->bitmap_left;
  bitmap.up_offset   = -glyph->bitmap_top;
  bitmap.data.resize(bitmap.extent.x * bitmap.extent.y);
  memcpy(bitmap.data.data(), ft_bitmap.buffer, bitmap.data.size());
  return bitmap;
}

}}