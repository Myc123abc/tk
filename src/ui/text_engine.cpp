#include "text_engine.hpp"
#include "util/error_handling.hpp"
#include "../renderer/resource/image_manager.hpp"
#include "missing-glyph-sdf-bitmap.hpp"
#include "../renderer/engine/copy_engine.hpp"

#include <hb-ft.h>
#include <utf8.h>

#include <ranges>

#define check(x, msg) err_if(static_cast<bool>(x), "[TextEngine] {}", msg)

using namespace tk::renderer;

namespace tk::ui {

void Font::init(std::string_view path) noexcept
{
  _name = path;

  check(FT_New_Face(g_text_engine._ft, path.data(), 0, &_face), "failed to load font");
  check(FT_Set_Pixel_Sizes(_face, 0, FT_Pixel_Size), "failed to set pixel size");

  _hb_font = hb_ft_font_create(_face, nullptr);

  _ascender = static_cast<float>(_face->ascender) * FT_Pixel_Size / _face->units_per_EM;
  _height   = static_cast<float>(_face->height)   * FT_Pixel_Size / _face->units_per_EM;

  using enum FontStyle;
  switch (_face->style_flags)
  {
  case FT_STYLE_FLAG_ITALIC:
    _style = italic;
    break;
  case FT_STYLE_FLAG_BOLD:
    _style = bold;
    break;
  case FT_STYLE_FLAG_ITALIC | FT_STYLE_FLAG_BOLD:
    _style = italic_bold;
    break;
  default:
    _style = regular;
    break;
  }
}

void Font::destroy() const noexcept
{
  hb_font_destroy(_hb_font);
  check(FT_Done_Face(_face), "failed to destroy font");
}

auto Font::generate_sdf_bitmap(uint glyph_idx, uint unicode, FontStyle style) const noexcept -> SDFBitmap
{
  assert(glyph_idx);
  check(FT_Load_Glyph(_face, glyph_idx, FT_LOAD_RENDER), "failed to load glyph with render");
  auto glyph = _face->glyph;
  check(FT_Render_Glyph(glyph, FT_RENDER_MODE_SDF), "failed to render sdf bitmap");
  auto ft_bitmap = _face->glyph->bitmap;
  auto bitmap = SDFBitmap{};
  bitmap.extent      = { ft_bitmap.width, ft_bitmap.rows };
  bitmap.unicode     = unicode;
  bitmap.style       = style;
  bitmap.left_offset = glyph->bitmap_left;
  bitmap.up_offset   = -glyph->bitmap_top;
  bitmap.data = g_text_engine._mem_pool.vector<uint8>();
  bitmap.data.resize(bitmap.extent.x * bitmap.extent.y);
  memcpy(bitmap.data.data(), ft_bitmap.buffer, bitmap.data.size());
  return bitmap;
}

void TextEngine::init() noexcept
{
  check(FT_Init_FreeType(&_ft), "failed to initialize freetype");
  _hb_buf = hb_buffer_create();

  // create the first glyph atlas
  _glyph_atlas.emplace_back(g_img_mgr.create(Glyph_Atlas_Width, Glyph_Atlas_Height, ImageFormat::r8_unorm, ImageType::srv));
}

void TextEngine::destroy() const noexcept
{
  for (auto handle : _glyph_atlas) g_img_mgr.destroy(handle);
  for (auto const& font : _fonts | std::views::values | std::views::join) font.destroy();

  hb_buffer_destroy(_hb_buf);
  check(FT_Done_FreeType(_ft), "failed to destroy freetype");
}

void TextEngine::load_font(std::string_view path) noexcept
{
  if (std::ranges::any_of(_fonts | std::views::values | std::views::join, [&](auto const& font) { return font.name() == path; }))
  {
    warn("[TextEngine] font {} is already loaded", path);
    return;
  }
  
  // create font
  auto font = Font{};
  font.init(path);
  _fonts[font.style()].emplace_back(std::move(font));

  // clear missing glyphs and cached text advances
  _missing_glyphs.clear();
  for (auto const& [style, text] : _cached_texts_with_missing_glyphs)
    _cached_text_advances[style].erase(text);
  _cached_texts_with_missing_glyphs.clear();
}

auto TextEngine::parse(std::string_view text, FontStyle style) noexcept -> ParseResult
{
  assert(!text.empty());

  auto res = ParseResult{};
  res.text = utf8::utf8to32(text);

  auto has_missing_glyphs = false;

  // try to get cached text advances
  auto& cached_text_advances = _cached_text_advances[style];
  if (cached_text_advances.contains(text.data())) return cached_text_advances[text.data()];

  // calculate advances
  res.advances.reserve(res.text.size());

  // split text
  for (auto [text, font] : split_text(res.text, style))
  {
    // use hb calculate advances
    if (font)
    {
      hb_buffer_reset(_hb_buf);
      hb_buffer_add_utf32(_hb_buf, reinterpret_cast<uint const*>(text.data()), text.size(), 0, -1);
      hb_buffer_guess_segment_properties(_hb_buf);
      hb_shape(font->_hb_font, _hb_buf, nullptr, 0);

      auto glyph_positions = hb_buffer_get_glyph_positions(_hb_buf, nullptr);
      for (auto i = 0; i < text.size(); ++i)
        res.advances.emplace_back(static_cast<float>(glyph_positions[i].x_advance) / 64, static_cast<float>(glyph_positions[i].y_advance) / 64);
    }
    // if not have font, the text is missing glyphs
    else
    {
      has_missing_glyphs = true;
      static auto missing_glyph_position_info = float2{ Missing_Glyph_Advance_X * Missing_Glyph_Size / FT_Pixel_Size,
                                                        Missing_Glyph_Advance_Y * Missing_Glyph_Size / FT_Pixel_Size };
      res.advances.resize(res.advances.size() + text.size(), missing_glyph_position_info);
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

  res.max_ascender = _max_ascender;
  res.max_height   = _max_height;

  // cached calculate result
  cached_text_advances.emplace(text.data(), res);

  add_uncached_glyphs(res.text, style);

  return std::move(res);
}

auto TextEngine::split_text(std::u32string_view text, FontStyle style) noexcept -> std::vector<std::pair<std::u32string_view, Font*>>
{
  std::vector<std::pair<std::u32string_view, Font*>> result;
  result.reserve(text.size());

  _max_ascender = {};
  _max_height   = {};

  auto update = [this](Font* font)
  {
    if (!font) return;
    _max_ascender = std::max(_max_ascender, font->_ascender);
    _max_height   = std::max(_max_height, font->_height);
  };

  auto beg      = text.begin();
  auto cur_font = find_font(*beg, style);;
  update(cur_font);

  for (auto it = beg + 1; it != text.end(); ++it)
  {
    auto font = find_font(*it, style);
    if (font == cur_font) continue;

    result.emplace_back(std::u32string_view{ beg, it }, cur_font);
    beg      = it;
    cur_font = font;
    update(cur_font);
  }

  assert(beg != text.end());
  result.emplace_back(std::u32string_view{ beg, text.end() }, cur_font);

  return result;
}

auto TextEngine::find_font(uint unicode, FontStyle style) noexcept -> Font*
{
  auto fonts = _fonts.find(style);
  if (fonts == _fonts.end()) return {};
  auto it = std::ranges::find_if(fonts->second, [unicode](auto& font) { return font.find_glyph(unicode); });
  return it == fonts->second.end() ? nullptr : &*it;
}

void TextEngine::add_uncached_glyphs(std::u32string_view text, FontStyle style) noexcept
{
  for (auto ch : text)
  {
    if (!glyph_infos_has(ch, style) && !missing_glyphs_has(ch, style) && !uncached_glyphs_has(ch, style))
    {
      if (auto res = find_glyph(ch, style))
        _uncached_glyphs[style].emplace(ch, res.value());
      else
        _missing_glyphs[style].emplace(ch);
    }
  }
}

auto TextEngine::find_glyph(uint unicode, FontStyle style) noexcept -> std::optional<std::pair<Font*, uint>>
{
  // promise unicode is not generated sdf bitmap
  assert(!glyph_infos_has(unicode, style));
  for (auto& font : _fonts[style])
    if (auto glyphs_idx = font.find_glyph(unicode))
      return std::make_pair(&font, glyphs_idx);
  return {};
}

auto TextEngine::calc_glyph_pos(float2 extent) noexcept -> std::pair<uint, float2>
{
  check(extent.x > Glyph_Atlas_Width || extent.y > Glyph_Atlas_Height,
        "too big glyph sdf bitmap, cannot be stored in glyph atlas");

  static float2 current_pos{};
  static float  current_line_max_glyph_height{};
  static uint   current_glyph_atlas_idx{};
  
  while (true)
  {
    auto max_pos = current_pos + extent;

    if (max_pos.x <= Glyph_Atlas_Width && max_pos.y <= Glyph_Atlas_Height)
    {
      auto pos = current_pos;
      current_pos.x = max_pos.x;
      current_line_max_glyph_height = std::max(current_line_max_glyph_height, extent.y);
      return { current_glyph_atlas_idx, pos };
    }

    if (max_pos.x > Glyph_Atlas_Width)
    {
      current_pos.x = {};
      current_pos.y += current_line_max_glyph_height;
      current_line_max_glyph_height = {};
      continue;
    }

    ++current_glyph_atlas_idx;
    // TODO: _new_glyph_atlas = true;
    _glyph_atlas.emplace_back(g_img_mgr.create(Glyph_Atlas_Width, Glyph_Atlas_Height, ImageFormat::r8_unorm, ImageType::srv));
    current_pos                   = {};
    current_line_max_glyph_height = {};
  }
}

void TextEngine::upload_uncached_glyphs() noexcept
{
  if (_uncached_glyphs.empty()) return;

  for (auto const& [style, infos] : _uncached_glyphs)
  {
    for (auto const& [unicode, pair] : infos)
    {
      auto [font, glyph_idx] = pair;
      auto bitmap = font->generate_sdf_bitmap(glyph_idx, unicode, style);
      auto [glyph_atlas_idx, cpy_pos] = calc_glyph_pos(bitmap.extent);
      _pending_copy_glyphs[glyph_atlas_idx].emplace_back(std::move(bitmap), cpy_pos);
    }
  }
  _uncached_glyphs.clear();

  for (auto const& [glyph_atlas_idx, bitmaps] : _pending_copy_glyphs)
  {
  }

  _pending_copy_glyphs.clear();
}

}
