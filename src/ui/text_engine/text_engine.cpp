#include "text_engine.hpp"
#include "util/error_handling.hpp"
#include "../../renderer/resource/image_manager.hpp"
#include "../../util/file_manager.hpp"
#include "../../renderer/engine/copy_engine.hpp"

#include <hb-ft.h>
#include <utf8.h>
#include FT_OUTLINE_H

#include <ranges>

#define check(x, msg) err_if(static_cast<bool>(x), "[TextEngine] {}", msg)

using namespace tk;
using namespace tk::ui;
using namespace tk::renderer;

namespace {

constexpr auto Notdef_Glyph_Unicode = std::numeric_limits<uint>::max();

struct FtContext
{
  double           scale{};
  msdfgen::Point2  position{};
  msdfgen::Shape   *shape{};
  msdfgen::Contour *contour{};
};

auto ftPoint2(const FT_Vector &vector, double scale) noexcept
{
  return msdfgen::Point2(scale*vector.x, scale*vector.y);
}

auto ftMoveTo(const FT_Vector *to, void *user) noexcept
{
  FtContext *context = reinterpret_cast<FtContext *>(user);
  if (!(context->contour && context->contour->edges.empty()))
    context->contour = &context->shape->addContour();
  context->position = ftPoint2(*to, context->scale);
  return 0;
}

auto ftLineTo(const FT_Vector *to, void *user) noexcept
{
  FtContext *context = reinterpret_cast<FtContext *>(user);
  auto endpoint = ftPoint2(*to, context->scale);
  if (endpoint != context->position)
  {
    context->contour->addEdge(msdfgen::EdgeHolder(context->position, endpoint));
    context->position = endpoint;
  }
  return 0;
}

auto ftConicTo(const FT_Vector *control, const FT_Vector *to, void *user) noexcept
{
  FtContext *context = reinterpret_cast<FtContext *>(user);
  auto endpoint = ftPoint2(*to, context->scale);
  if (endpoint != context->position)
  {
    context->contour->addEdge(msdfgen::EdgeHolder(context->position, ftPoint2(*control, context->scale), endpoint));
    context->position = endpoint;
  }
  return 0;
}

auto ftCubicTo(const FT_Vector *control1, const FT_Vector *control2, const FT_Vector *to, void *user) noexcept
{
  FtContext *context = reinterpret_cast<FtContext *>(user);
  auto endpoint = ftPoint2(*to, context->scale);
  if (endpoint != context->position || crossProduct(ftPoint2(*control1, context->scale)-endpoint, ftPoint2(*control2, context->scale)-endpoint))
  {
    context->contour->addEdge(msdfgen::EdgeHolder(context->position, ftPoint2(*control1, context->scale), ftPoint2(*control2, context->scale), endpoint));
    context->position = endpoint;
  }
  return 0;
}

auto get_shape(FT_GlyphSlot glyph, uint glyph_idx, double scale) noexcept
{
  // read outline
  auto shape = msdfgen::Shape{};
  shape.setYAxisOrientation(msdfgen::Y_DOWNWARD);

  auto ctx = FtContext{};
  ctx.scale = scale;
  ctx.shape = &shape;

  auto outline_funcs = FT_Outline_Funcs{};
  outline_funcs.move_to  = &ftMoveTo;
  outline_funcs.line_to  = &ftLineTo;
  outline_funcs.conic_to = &ftConicTo;
  outline_funcs.cubic_to = &ftCubicTo;
  outline_funcs.shift    = 0;
  outline_funcs.delta    = 0;
  auto err = FT_Outline_Decompose(&glyph->outline, &outline_funcs, &ctx);
  assert(!err);

  if (!shape.contours.empty() && shape.contours.back().edges.empty())
    shape.contours.pop_back();

  // validate and normalize shape
  assert(shape.validate());
  shape.normalize();

  return std::move(shape);
}

auto generate_msdf(msdfgen::Shape& shape) noexcept -> std::pair<msdfgen::Bitmap<float, 3>, float2>
{
  if (shape.contours.empty()) return {};

  static auto const px_range      = msdfgen::Range{ Glyph_MSDF_Pixel_Range };
  static auto const scale         = msdfgen::Vector2{ FT_Pixel_Size };
  static auto const range         = px_range / std::min(scale.x, scale.y);
  static auto const sd_zero_value = range.lower != range.upper ? float(range.lower/(range.lower-range.upper)) : .5f;

  static auto const generator_cfg        = msdfgen::MSDFGeneratorConfig{ true };
  static auto const post_err_correct_cfg = msdfgen::MSDFGeneratorConfig{ true,
    msdfgen::ErrorCorrectionConfig{ msdfgen::ErrorCorrectionConfig::DISABLED, msdfgen::ErrorCorrectionConfig::DO_NOT_CHECK_DISTANCE }};

  auto bounds    = shape.getBounds();
  auto min_x     = bounds.l + range.lower;
  auto min_y     = bounds.b + range.lower;
  auto max_x     = bounds.r + range.upper;
  auto max_y     = bounds.t + range.upper;
  auto width     = std::ceil((max_x - min_x) * scale.x);
  auto height    = std::ceil((max_y - min_y) * scale.y);
  auto translate = msdfgen::Vector2{ -min_x, -min_y };
  auto offset    = float2{ static_cast<float>(min_x * scale.x), static_cast<float>(-max_y * scale.y) };

  // generate msdf
  auto msdf           = msdfgen::Bitmap<float, 3>(width, height);
  auto transformation = msdfgen::SDFTransformation{ msdfgen::Projection{ scale, translate }, range};
  msdfgen::edgeColoringSimple(shape, 3, 0);
  msdfgen::generateMSDF(msdf, shape, transformation, generator_cfg);

  msdfgen::distanceSignCorrection(msdf, shape, transformation, sd_zero_value, msdfgen::FILL_NONZERO);
  msdfgen::msdfErrorCorrection(msdf, shape, transformation, post_err_correct_cfg);

  return { std::move(msdf), offset };
}

}

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
  switch (_face->style_flags & 0b11)
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

auto Font::generate_msdf_bitmap(uint glyph_idx, GlyphKey key) const noexcept -> MSDFBitmap
{
  // load glyph
  check(FT_Load_Glyph(_face, glyph_idx, FT_LOAD_NO_SCALE), "failed to load glyph no scale");

  // calc scale
  auto scale = 1.0 / (_face->units_per_EM ? _face->units_per_EM : 1);

  // generate msdf bitmap
  auto shape              = get_shape(_face->glyph, glyph_idx, scale);
  auto [msdf, pos_offset] = generate_msdf(shape);

  auto bitmap = MSDFBitmap{};
  bitmap.extent     = { msdf.width(), msdf.height() };
  bitmap.glyph_key  = key;
  bitmap.pos_offset = pos_offset;
  bitmap.data.resize(bitmap.extent.x * bitmap.extent.y * 4);

  auto cnt  = bitmap.extent.x * bitmap.extent.y;
  auto data = static_cast<float*>(msdf);
  for (auto i = 0; i < cnt; ++i)
  {
    auto to = [](float v) -> uint8 { return std::clamp(v, 0.f, 1.f) * 255.f + .5f; };
    bitmap.data[i * 4 + 0] = to(data[i * 3 + 0]);
    bitmap.data[i * 4 + 1] = to(data[i * 3 + 1]);
    bitmap.data[i * 4 + 2] = to(data[i * 3 + 2]);
    bitmap.data[i * 4 + 3] = 255;
  }

  return bitmap;
}

auto Font::get_glyph_advance(uint glyph_idx) const noexcept -> float2
{
  check(FT_Load_Glyph(_face, glyph_idx, FT_LOAD_DEFAULT), "failed to load glyph for advance");
  auto glyph = _face->glyph;
  return { static_cast<float>(glyph->advance.x) / 64, static_cast<float>(glyph->advance.y) / 64 };
}

void TextEngine::init() noexcept
{
  check(FT_Init_FreeType(&_ft), "failed to initialize freetype");
  _hb_buf = hb_buffer_create();

  // create the first glyph atlas
  _glyph_atlas.emplace_back(g_img_mgr.create(Glyph_Atlas_Width, Glyph_Atlas_Height, ImageFormat::rgba8_unorm, ImageType::srv));
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
  auto info  = FontInfo{ font._family, font._style };
  auto style = font.style();
  _fonts[style].emplace_back(std::move(font));

  // clear missing glyphs and cached text advances
  _missing_glyphs[style].clear();
  for (auto hash : _cached_texts_with_missing_glyphs[style])
  {
    auto h = _cached_text_parse_results[style][hash];
    assert(h.valid());
    _discard_text_parse_result_handles.emplace_back(h);
    _cached_text_parse_results[style].erase(hash);
  }
  _cached_texts_with_missing_glyphs[style].clear();

  // clear notdef missing font
  // TODO: when expand glyph release feature, need to process old notdef glyph release too
  if (_missing_notdef_font_styles.erase(style))
  {
    auto k = GlyphKey{ style, Notdef_Glyph_Unicode };
    _glyph_infos.erase(k);
    _uncached_glyphs.erase(k);
  }

  return info;
}

auto TextEngine::parse(std::string_view text, FontStyle style, std::string_view family) noexcept -> TextParseResultHandle
{
  assert(!text.empty());

  auto hash = std::hash<std::string_view>{}(text);

  // try to get cached text advances
  auto& cached_text_parse_result = _cached_text_parse_results[style];
  if (cached_text_parse_result.contains(hash)) return cached_text_parse_result[hash];

  auto res    = ParseResult{};
  auto u32str = utf8::utf8to32(text);

  auto has_missing_glyphs = false;

  // calculate advances
  res.advances.reserve(u32str.size());
  res.glyph_info_keys.reserve(u32str.size());

  // split text
  for (auto [text, font] : split_text(u32str, style))
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
        // TODO: harfbuzz usage maybe wrong, test a text which have offset
        auto x = static_cast<float>(glyph_positions[i].x_advance) / 64;
        auto y = static_cast<float>(glyph_positions[i].y_advance) / 64;
        res.advances.emplace_back(x, y);
        res.glyph_info_keys.emplace_back(style, text[i]);
        res.extent.x += x;
      }
    }
    // if not have font, the text is missing glyphs
    else
    {
      has_missing_glyphs = true;
      auto notdef_glyph_font = find_notdef_glyph_font(style).first;
      assert(notdef_glyph_font);
      auto notdef_glyph_advance = notdef_glyph_font->get_glyph_advance(0);
      auto k = GlyphKey{ style, Notdef_Glyph_Unicode };
      res.advances.resize(res.advances.size() + text.size(), notdef_glyph_advance);
      res.glyph_info_keys.resize(res.glyph_info_keys.size() + text.size(), k);
      res.extent.x += text.size() * notdef_glyph_advance.x;
      _max_ascender = std::max(_max_ascender, notdef_glyph_font->_ascender);
      _max_height   = std::max(_max_height,   notdef_glyph_font->_height);
      if (add_notdef_glyph(style)) res.generating_glyph_info_keys.emplace(k);
    }
  }

  if (has_missing_glyphs) 
    _cached_texts_with_missing_glyphs[style].emplace_back(hash);

  res.ascender = _max_ascender;
  res.extent.y = _max_height;

  // cached calculate result
  assert(!cached_text_parse_result[hash].valid());
  auto handle = _parse_result_pool.alloc();
  cached_text_parse_result[hash] = handle;

  add_uncached_glyphs(u32str, style, res);
  if (!res.generating_glyph_info_keys.empty()) _generating_results.emplace(handle);

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

auto font_style_str(FontStyle style) noexcept
{
  switch (style)
  {
  case FontStyle::regular:     return "regular";
  case FontStyle::bold:        return "bold";
  case FontStyle::italic:      return "italic";
  case FontStyle::italic_bold: return "italic_bold";
  default: std::unreachable();
  }
}

auto TextEngine::find_notdef_glyph_font(FontStyle style) noexcept -> std::pair<Font*, bool>
{
  if (auto fonts = _fonts.find(style); fonts != _fonts.end() && !fonts->second.empty())
    return { &fonts->second.front(), false };

  if (auto fonts = _fonts.find(FontStyle::regular); fonts != _fonts.end() && !fonts->second.empty())
    return { &fonts->second.front(), true };

  for (auto& [font_style, fonts] : _fonts)
    if (!fonts.empty()) return { &fonts.front(), true };

  err_if(true, "there are not have any font be loaded!");
  std::unreachable();
}

void TextEngine::add_uncached_glyphs(std::u32string_view text, FontStyle style, ParseResult& result) noexcept
{
  for (auto ch : text)
  {
    auto k = GlyphKey{ style, ch };
    if (!_glyph_infos.contains(k) && !_missing_glyphs[style].contains(ch))
    {
      if (_uncached_glyphs.contains(k))
      {
        result.generating_glyph_info_keys.emplace(k);
        continue;
      }

      if (auto res = find_glyph(ch, style))
      {
        result.generating_glyph_info_keys.emplace(k);
        _uncached_glyphs.emplace(k);
        _ungenerated_glyphs.emplace(k, std::move(res.value()));
      }
      else
        _missing_glyphs[style].emplace(ch);
    }
  }
}

auto TextEngine::add_notdef_glyph(FontStyle style) noexcept -> bool
{
  auto k = GlyphKey{ style, Notdef_Glyph_Unicode };
  if (_glyph_infos.contains(k)) return false;
  if (_uncached_glyphs.contains(k)) return true;

  auto [font, is_fallback] = find_notdef_glyph_font(style);
  if (!font) return false;
  if (is_fallback) _missing_notdef_font_styles.emplace(style);

  _uncached_glyphs.emplace(k);
  _ungenerated_glyphs.emplace(k, std::make_pair(font, 0));
  return true;
}

auto TextEngine::find_glyph(uint unicode, FontStyle style) noexcept -> std::optional<std::pair<Font*, uint>>
{
  // promise unicode is not generated sdf bitmap
  assert(!_glyph_infos.contains(GlyphKey{ style, unicode }));
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
    _glyph_atlas.emplace_back(g_img_mgr.create(Glyph_Atlas_Width, Glyph_Atlas_Height, ImageFormat::rgba8_unorm, ImageType::srv));
    current_pos                   = {};
    current_line_max_glyph_height = {};
  }
}

auto TextEngine::get_notdef_glyph_info(FontStyle style) noexcept -> GlyphInfo const&
{
  auto k = GlyphKey{ style, Notdef_Glyph_Unicode };
  if (_glyph_infos.contains(k)) return _glyph_infos.at(k);

  for (auto [key, info] : _glyph_infos)
    if (k.has(Notdef_Glyph_Unicode)) return info;

  std::unreachable();
}

void TextEngine::submit_bitmap_generation_tasks() noexcept
{
  if (_ungenerated_glyphs.empty()) return;

  // submit msdf bitmap generate task on thread pool
  _generate_bitmap_tasks.emplace_back(g_thread_pool.submit([glyphs = std::move(_ungenerated_glyphs)]
  {
    auto bitmaps = std::vector<MSDFBitmap>{};
    bitmaps.reserve(glyphs.size());
    debug("generate glyphs count : {}", bitmaps.capacity());
    auto beg = std::chrono::high_resolution_clock::now();
    for (auto const& [k, pair] : glyphs)
      // font should always be valid because I never remove font currently
      bitmaps.emplace_back(pair.first->generate_msdf_bitmap(pair.second, k));
    auto dur = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - beg).count();
    debug("consume {}ms", dur);
    return bitmaps;
  }));
}

void TextEngine::upload_bitmaps() noexcept
{
  if (_generate_bitmap_tasks.empty()) return;

  // check whether have task complete
  for (auto it = _generate_bitmap_tasks.begin(); it < _generate_bitmap_tasks.end();)
  {
    if (it->is_completed())
    {
      for (auto const& bitmap : it->take_result())
      {
        auto [glyph_atlas_idx, cpy_pos] = calc_glyph_pos(bitmap.extent);
        _pending_copy_glyphs[glyph_atlas_idx].emplace_back(std::move(bitmap), cpy_pos);
      }
      it = _generate_bitmap_tasks.erase(it);
    }
    else
      ++it;
  }

  if (_pending_copy_glyphs.empty()) return;

  // use copy engine to upload glyph sdf bitmaps to glyph atlas texture
  for (auto const& [glyph_atlas_idx, bitmap_infos] : _pending_copy_glyphs)
  {
    for (auto const& [bitmap, pos] : bitmap_infos)
    {
      auto res = _glyph_infos.emplace(bitmap.glyph_key, GlyphInfo{ glyph_atlas_idx, pos, bitmap.extent, bitmap.pos_offset });
      assert(res.second);
    }

    auto bitmap_cpy_infos = bitmap_infos
      | std::views::filter([](auto const& bitmap_info) { return !bitmap_info.first.empty(); })
      | std::views::transform([](auto const& bitmap_info)
        { return BitmapCopyInfo{ bitmap_info.first.to_bitmap_view(), bitmap_info.second }; })
      | std::ranges::to<std::vector<BitmapCopyInfo>>();
    g_copy_engine.copy(std::move(bitmap_cpy_infos), _glyph_atlas[glyph_atlas_idx]);
  }

  // check which text parse result genrating is complete
  for (auto it = _generating_results.begin(); it != _generating_results.end();)
  {
    auto& result = _parse_result_pool[*it];
    assert(!result.generating_glyph_info_keys.empty());
    if (std::ranges::any_of(result.generating_glyph_info_keys, [&](auto k) { return !_glyph_infos.contains(k); }))
    {
      ++it;
      continue;
    }
    result.generating_glyph_info_keys.clear();
    it = _generating_results.erase(it);
  }

  // remove generated glyphs in uncached glyphs
  for (auto it = _uncached_glyphs.begin(); it != _uncached_glyphs.end();)
    _glyph_infos.contains(*it) ? it = _uncached_glyphs.erase(it) : ++it;
}

void GlyphInfo::set_vertices(renderer::Vertex* vtx, float2 pos, float size, float ascender, Color color, Color outer_color, float outer_width) const noexcept
{
  auto scale = get_scale(size);
  
  auto p0 = pos + pos_offset * scale;
  p0.y += ascender * scale;
  auto p1 = float2{ p0.x + extent.x * scale, p0.y };
  auto p3 = float2{ p0.x, p0.y + extent.y * scale };
  auto p2 = float2{ p1.x, p3.y };
  
  auto desc_handle = g_img_mgr[g_text_engine._glyph_atlas[glyph_atlas_index]].srv();
  assert(desc_handle.is_valid());

  auto col       = color.to_uint();
  auto outer_col = outer_color.to_uint();
  auto packed    = vtx_pack(VtxType::text, desc_handle.index());
  vtx[0] = { p0, { min_x, min_y }, col, packed, outer_col, outer_width };
  vtx[1] = { p1, { max_x, min_y }, col, packed, outer_col, outer_width };
  vtx[2] = { p2, { max_x, max_y }, col, packed, outer_col, outer_width };
  vtx[3] = { p3, { min_x, max_y }, col, packed, outer_col, outer_width };
}

void TextEngine::postprocess() noexcept
{
  for (auto h : _discard_text_parse_result_handles)
    _parse_result_pool.free(h);
  _discard_text_parse_result_handles.clear();
}

}
