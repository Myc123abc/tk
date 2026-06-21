#include "text_engine.hpp"
#include "util/error_handling.hpp"
#include "../../renderer/resource/image_manager.hpp"
#include "missing-glyph-sdf-bitmap.hpp"
#include "../../util/file_manager.hpp"
#include "../../renderer/engine/copy_engine.hpp"

#include <hb-ft.h>
#include <utf8.h>

#include <ranges>

#define check(x, msg) err_if(static_cast<bool>(x), "[TextEngine] {}", msg)

using namespace tk::renderer;

namespace tk::ui {

auto Font::init(std::string_view path) noexcept -> uint8
{
  _name = path;

  auto res = FT_New_Face(g_text_engine._ft, path.data(), 0, &_face);
  if (res) return res;
  res = FT_Set_Pixel_Sizes(_face, 0, FT_Pixel_Size);
  if (res) return res;

  _hb_font = hb_ft_font_create(_face, nullptr);

  _ascender = static_cast<float>(_face->ascender) * FT_Pixel_Size / _face->units_per_EM;
  _height   = static_cast<float>(_face->height)   * FT_Pixel_Size / _face->units_per_EM;
  _family   = _face->family_name;

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

  return res;
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
  check(FT_Render_Glyph(glyph, FT_RENDER_MODE_NORMAL), "failed to render bitmap");
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

void TextEngine::destroy() noexcept
{
  for (auto handle : _glyph_atlas) g_img_mgr.destroy(handle);
  for (auto const& font : _fonts | std::views::values | std::views::join) font.destroy();
  for (auto& h : _cached_text_parse_results | std::views::values | std::views::join | std::views::values)
  {
    assert(h.valid());
    _parse_result_pool.free(h);
  }

  hb_buffer_destroy(_hb_buf);
  check(FT_Done_FreeType(_ft), "failed to destroy freetype");

  assert(_discard_text_parse_result_handles.empty());
}

auto TextEngine::load_font(std::string_view path) noexcept -> std::expected<FontInfo, FontLoadErrorType>
{
  // check whether already loaded
  auto fonts = _fonts | std::views::values | std::views::join;
  if (auto it = std::ranges::find_if(fonts, [&](auto const& font) { return font.name() == path; });
      it != fonts.end())
    return FontInfo{ it->_family, it->_style };

  // check whether exist
  if (!g_file_mgr.exists(path)) return std::unexpected(FontLoadError::unexist{});
  
  // create font
  auto font = Font{};
  if (auto res = font.init(path); res) return std::unexpected(FontLoadError::freetype_err{ res });
  auto info = FontInfo{ font._family, font._style };
  _fonts[font.style()].emplace_back(std::move(font));

  // clear missing glyphs and cached text advances
  _missing_glyphs[font.style()].clear();
  for (auto const& [style, text] : _cached_texts_with_missing_glyphs)
  {
    auto h = _cached_text_parse_results[style][text.data()];
    assert(h.valid());
    _discard_text_parse_result_handles.emplace_back(h);
    _cached_text_parse_results[style].erase(text);
  }
  _cached_texts_with_missing_glyphs.clear();

  return info;
}

void TextEngine::clear_discard_text_parse_results() noexcept
{
  for (auto h : _discard_text_parse_result_handles) _parse_result_pool.free(h);
  _discard_text_parse_result_handles.clear();
}

auto TextEngine::parse(std::string_view text, FontStyle style, std::string_view family) noexcept -> TextParseResultHandle
{
  assert(!text.empty());

  auto res = ParseResult{};
  res.text = utf8::utf8to32(text);

  auto has_missing_glyphs = false;

  // try to get cached text advances
  auto& cached_text_parse_result = _cached_text_parse_results[style];
  if (cached_text_parse_result.contains(text.data())) return cached_text_parse_result[text.data()];

  // calculate advances
  res.advances.reserve(res.text.size());
  res.style = style;

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
      {
        auto x = static_cast<float>(glyph_positions[i].x_advance) / 64;
        auto y = static_cast<float>(glyph_positions[i].y_advance) / 64;
        res.advances.emplace_back(x, y);
        res.extent.x += x;
      }
    }
    // if not have font, the text is missing glyphs
    else
    {
      has_missing_glyphs = true;
      static auto missing_glyph_position_info = float2{ Missing_Glyph_Advance_X * Missing_Glyph_Size / FT_Pixel_Size,
                                                        Missing_Glyph_Advance_Y * Missing_Glyph_Size / FT_Pixel_Size };
      res.advances.resize(res.advances.size() + text.size(), missing_glyph_position_info);
      res.extent.x += res.advances.size() * missing_glyph_position_info.x;
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

  res.ascender = _max_ascender;
  res.extent.y = _max_height;

  // cached calculate result
  assert(!cached_text_parse_result[text.data()].valid());
  auto handle = _parse_result_pool.alloc();
  cached_text_parse_result[text.data()] = handle;

  add_uncached_glyphs(res.text, style);

  _parse_result_pool[handle] = std::move(res);

  return handle;
}

auto TextEngine::split_text(std::u32string_view text, FontStyle style) noexcept -> std::vector<std::pair<std::u32string_view, Font*>>
{
  // get which text use which font
  auto result = std::vector<std::pair<std::u32string_view, Font*>>{};
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
    _glyph_atlas.emplace_back(g_img_mgr.create(Glyph_Atlas_Width, Glyph_Atlas_Height, ImageFormat::r8_unorm, ImageType::srv));
    current_pos                   = {};
    current_line_max_glyph_height = {};
  }
}

auto TextEngine::get_missing_glyph_info() noexcept -> GlyphInfo const&
{
  return _glyph_infos[FontStyle::regular].at(Missing_Glyph_Unicode);
}

void TextEngine::upload_uncached_glyphs() noexcept
{
  auto& pending_copy_glyphs = _pending_copy_glyphs.data();

  // cache missing glyph
  if (!_glyph_infos[FontStyle::regular].contains(Missing_Glyph_Unicode))
  {
    // copy missing glyph sdf bitmap
    auto bitmap = SDFBitmap{};
    bitmap.data.resize(sizeof(Missing_Glyph_SDF_Bitmap));
    memcpy(bitmap.data.data(), Missing_Glyph_SDF_Bitmap, bitmap.data.size());
    bitmap.extent      = { Missing_Glyph_Width, Missing_Glyph_Height };
    bitmap.unicode     = Missing_Glyph_Unicode;
    bitmap.style       = FontStyle::regular;
    bitmap.left_offset = Missing_Glyph_Left_Offset;
    bitmap.up_offset   = Missing_Glyph_Up_Offset;

    // add to pending copy glyphs
    auto [glyph_atlas_idx, cpy_pos] = calc_glyph_pos({ Missing_Glyph_Width, Missing_Glyph_Height });
    pending_copy_glyphs[glyph_atlas_idx].emplace_back(std::move(bitmap), cpy_pos);
  }

  if (_uncached_glyphs.empty()) return;

  for (auto const& [style, infos] : _uncached_glyphs)
  {
    for (auto const& [unicode, pair] : infos)
    {
      auto [font, glyph_idx] = pair;
      auto bitmap = font->generate_sdf_bitmap(glyph_idx, unicode, style);
      auto [glyph_atlas_idx, cpy_pos] = calc_glyph_pos(bitmap.extent);
      pending_copy_glyphs[glyph_atlas_idx].emplace_back(std::move(bitmap), cpy_pos);
    }
  }
  _uncached_glyphs.clear();

  // use copy engine to upload glyph sdf bitmaps to glyph atlas texture
  for (auto const& [glyph_atlas_idx, bitmap_infos] : pending_copy_glyphs)
  {
    for (auto const& [bitmap, pos] : bitmap_infos)
      _glyph_infos[bitmap.style].emplace(bitmap.unicode, GlyphInfo{ glyph_atlas_idx, pos, bitmap.extent, bitmap.left_offset, bitmap.up_offset });

    auto bitmap_cpy_infos = bitmap_infos
      | std::views::filter([](auto const& bitmap_info) { return !bitmap_info.first.empty(); })
      | std::views::transform([](auto const& bitmap_info)
        { return BitmapCopyInfo{ bitmap_info.first.to_bitmap_view(), bitmap_info.second }; })
      | std::ranges::to<std::vector<BitmapCopyInfo>>();
    g_copy_engine.copy(std::move(bitmap_cpy_infos), _glyph_atlas[glyph_atlas_idx]);
  }

  _pending_copy_glyphs.swap();
}

void GlyphInfo::set_vertices(renderer::Vertex* vtx, float2 pos, float size, float ascender, Color color) const noexcept
{
  auto scale = get_scale(size);
  
  auto p0 = pos + pos_offset * scale;
  p0.y += ascender * scale;
  auto p1 = float2{ p0.x + extent.x * scale, p0.y };
  auto p3 = float2{ p0.x, p0.y + extent.y * scale };
  auto p2 = float2{ p1.x, p3.y };
  
  auto desc_handle = g_img_mgr[g_text_engine._glyph_atlas[glyph_atlas_index]].srv();
  assert(desc_handle.is_valid());
  auto idx = static_cast<uint>(desc_handle.index());

  auto col = color.to_uint();
  vtx[0] = { p0, { min_x, min_y }, col, idx };
  vtx[1] = { p1, { max_x, min_y }, col, idx };
  vtx[2] = { p2, { max_x, max_y }, col, idx };
  vtx[3] = { p3, { min_x, max_y }, col, idx };
}

}
