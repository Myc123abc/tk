#include "text_engine.hpp"
#include "tk/error_handling.hpp"
#include "../../renderer/resource/image_manager.hpp"
#include "../../util/file_manager.hpp"
#include "../../renderer/engine/copy_engine.hpp"
#include "glyph_cacher.hpp"

#include <utf8.h>

#include <ranges>

#define check(x, msg) err_if(static_cast<bool>(x), "[TextEngine] {}", msg)

using namespace tk;
using namespace tk::renderer;

namespace tk::ui {

void TextEngine::init() noexcept
{
  _packer.reset({ Glyph_Atlas_Width, Glyph_Atlas_Height });

  check(FT_Init_FreeType(&_ft), "failed to initialize freetype");
  _hb_buf = hb_buffer_create();

  // create the first glyph atlas
  _glyph_atlas.emplace_back(g_img_mgr.create(Glyph_Atlas_Width, Glyph_Atlas_Height, ImageFormat::rgba8_unorm, ImageType::srv));
}

void TextEngine::destroy() noexcept
{
  for (auto handle : _glyph_atlas) g_img_mgr.destroy(handle);
  for (auto const& font : _fonts) font->destroy();
  for (auto& h : _cached_text_parse_results | std::views::values)
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
  if (auto it = std::ranges::find_if(_fonts, [&](auto const& font) { return font->name() == path; });
      it != _fonts.end())
    return FontInfo{ it->get()->_family, it->get()->_style };

  // check whether exist
  if (!g_file_mgr.exists(path)) return std::unexpected(FontLoadError::unexist{});

  // create font
  auto font = std::make_unique<Font>();
  if (auto res = font->init(path); res) return std::unexpected(FontLoadError::freetype_err{ res });

  // create font info
  font->_id = static_cast<uint>(_fonts.size());
  auto info = FontInfo{ font->_family, font->_style };

  // store font
  auto loaded_font = _fonts.emplace_back(std::move(font)).get();
  auto res = _font_idxs.emplace(loaded_font->key(), loaded_font->id());
  assert(res.second);
  _style_font_idxs[static_cast<size_t>(loaded_font->style())].emplace_back(loaded_font->id());

  // set notdef font if not have
  if (!_notdef_font) _notdef_font = loaded_font;
  
  for (auto const& [key, missing_glyphs] : _missing_glyphs)
    if (key.style == loaded_font->style())
      regenerate_missing_glyphs(loaded_font, key);

  return info;
}

void TextEngine::regenerate_missing_glyphs(Font* font, FontStyleKey key) noexcept
{
  auto& missing_glyphs = _missing_glyphs[key];
  if (missing_glyphs.empty()) return;

  auto& glyphs = _pending_reload_missing_glyphs[key];

  for (auto unicode : missing_glyphs)
  {
    auto glyph_idx = font->find_glyph(unicode);
    if (!glyph_idx) continue;

    _fallback_font_idxs[key][unicode] = font->id();
    auto key = GlyphKey{ font->id(), glyph_idx };
    glyphs.emplace(key);
    assert(!_glyph_infos.contains(key) && !_uncached_glyphs.contains(key));
    _ungenerated_glyphs.emplace(key);
  }
}

auto TextEngine::parse(std::string_view text, FontStyle style, std::string_view family, TextDirection direction) noexcept -> TextParseResultHandle
{
  assert(!text.empty());

  auto text_hash = generic_hash(text);
  auto key       = TextEngine::ParseKey{ text_hash, generic_hash(family), style, direction };

  // try to get cached text advances
  if (auto it = _cached_text_parse_results.find(key); it != _cached_text_parse_results.end())
  {
    auto const& result = _parse_result_pool[it->second];
    if (result.generating_glyph_info_keys.empty())
    {
      _last_ready_text_parse_results[text_hash] = it->second;
      return it->second;
    }
    if (auto fallback_it = _last_ready_text_parse_results.find(text_hash); fallback_it != _last_ready_text_parse_results.end())
      return fallback_it->second;
    return it->second;
  }

  auto res    = ParseResult{};
  auto u32str = utf8::utf8to32(text);

  auto has_missing_glyphs = false;

  // calculate advances
  res.advances.reserve(u32str.size());
  res.offsets.reserve(u32str.size());
  res.glyph_info_keys.reserve(u32str.size());

  // split text
  auto glyph_style_key = FontStyleKey(family, style);
  for (auto [text, font] : split_text(u32str, glyph_style_key))
  {
    // use hb calculate advances
    if (font)
    {
      {
        std::lock_guard lock{ font->_mutex };
        hb_buffer_reset(_hb_buf);
        hb_buffer_add_utf32(_hb_buf, reinterpret_cast<uint const*>(text.data()), text.size(), 0, -1);
        if (direction == TextDirection::vertical)
          hb_buffer_set_direction(_hb_buf, HB_DIRECTION_TTB);
        hb_buffer_guess_segment_properties(_hb_buf);
        hb_shape(font->_hb_font, _hb_buf, nullptr, 0);
      }

      auto glyph_count     = hb_buffer_get_length(_hb_buf);
      auto glyph_infos     = hb_buffer_get_glyph_infos(_hb_buf, nullptr);
      auto glyph_positions = hb_buffer_get_glyph_positions(_hb_buf, nullptr);
      for (auto i = 0u; i < glyph_count; ++i)
      {
        auto advance = float2{
          static_cast<float>(glyph_positions[i].x_advance) / 64,
          static_cast<float>(glyph_positions[i].y_advance) / 64,
        };
        auto offset = float2{
          static_cast<float>(glyph_positions[i].x_offset) / 64,
          static_cast<float>(glyph_positions[i].y_offset) / 64,
        };
        if (direction == TextDirection::vertical)
        {
          advance.y = -advance.y;
          offset.y  = -offset.y;
        }
        auto key = GlyphKey{ font->id(), glyph_infos[i].codepoint };

        res.advances.emplace_back(advance);
        res.offsets.emplace_back(offset);
        res.glyph_info_keys.emplace_back(key);
        if (direction == TextDirection::horizontal)
          res.extent.x += advance.x;
        else
          res.extent.y += advance.y;
        add_uncached_glyph(font, glyph_infos[i].codepoint, key, res);
      }
    }
    // if not have font, the text is missing glyphs
    else
    {
      has_missing_glyphs = true;
      auto notdef_glyph_font = find_notdef_glyph_font();
      assert(notdef_glyph_font);

      {
        std::lock_guard lock{ notdef_glyph_font->_mutex };
        hb_buffer_reset(_hb_buf);
        hb_buffer_add_utf32(_hb_buf, reinterpret_cast<uint const*>(text.data()), text.size(), 0, -1);
        if (direction == TextDirection::vertical)
          hb_buffer_set_direction(_hb_buf, HB_DIRECTION_TTB);
        hb_buffer_guess_segment_properties(_hb_buf);
        hb_shape(notdef_glyph_font->_hb_font, _hb_buf, nullptr, 0);
      }

      auto glyph_count     = hb_buffer_get_length(_hb_buf);
      auto glyph_positions = hb_buffer_get_glyph_positions(_hb_buf, nullptr);
      auto k = GlyphKey{ notdef_glyph_font->id(), 0 };
      for (auto i = 0u; i < glyph_count; ++i)
      {
        auto advance = float2{
          static_cast<float>(glyph_positions[i].x_advance) / 64,
          static_cast<float>(glyph_positions[i].y_advance) / 64,
        };
        auto offset = float2{
          static_cast<float>(glyph_positions[i].x_offset) / 64,
          static_cast<float>(glyph_positions[i].y_offset) / 64,
        };
        if (direction == TextDirection::vertical)
        {
          advance.y = -advance.y;
          offset.y  = -offset.y;
        }

        res.advances.emplace_back(advance);
        res.offsets.emplace_back(offset);
        res.glyph_info_keys.emplace_back(k);
        if (direction == TextDirection::horizontal)
          res.extent.x += advance.x;
        else
          res.extent.y += advance.y;
      }
      _max_ascender = std::max(_max_ascender, notdef_glyph_font->_ascender);
      _max_height   = std::max(_max_height,   notdef_glyph_font->_height);
      if (add_notdef_glyph()) res.generating_glyph_info_keys.emplace(k);
    }
  }

  if (has_missing_glyphs)
    _cached_texts_with_missing_glyphs[glyph_style_key].emplace_back(key);

  if (direction == TextDirection::horizontal)
  {
    res.extent.y = _max_height;
    res.ascender = _max_ascender;
  }
  else
  {
    res.is_vertical = true;
    res.extent.x    = _max_height;
    res.ascender    = res.extent.x * .5f;
    for (auto& offset : res.offsets)
      offset.x += res.ascender;
  }

  // cached calculate result
  auto handle = _parse_result_pool.alloc();
  auto [_, inserted] = _cached_text_parse_results.emplace(key, handle);
  assert(inserted);

  auto const is_generating = !res.generating_glyph_info_keys.empty();
  if (is_generating) _generating_results.emplace(handle);

  _parse_result_pool[handle] = std::move(res);

  if (is_generating)
  {
    if (auto fallback_it = _last_ready_text_parse_results.find(text_hash); fallback_it != _last_ready_text_parse_results.end())
      return fallback_it->second;
  }
  else
    _last_ready_text_parse_results[text_hash] = handle;

  return handle;
}

auto TextEngine::split_text(std::u32string_view text, FontStyleKey key) noexcept -> std::vector<std::pair<std::u32string_view, Font*>>
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
  auto cur_font = find_font(*beg, key);;
  update(cur_font);

  for (auto it = beg + 1; it != text.end(); ++it)
  {
    auto font = find_font(*it, key);
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

auto TextEngine::find_font(uint unicode, FontStyleKey key) noexcept -> Font*
{
  if (_missing_glyphs[key].contains(unicode)) return {};

  auto it = _font_idxs.find(key);
  if (it != _font_idxs.end())
  {
    auto font = _fonts[it->second].get();
    if (font->find_glyph(unicode)) return font;
  }

  auto& fallback_font_idxs = _fallback_font_idxs[key];
  if (auto fallback_it = fallback_font_idxs.find(unicode); fallback_it != fallback_font_idxs.end())
    return _fonts[fallback_it->second].get();

  for (auto font_idx : _style_font_idxs[static_cast<size_t>(key.style)])
  {
    auto font = _fonts[font_idx].get();
    if (!font->find_glyph(unicode)) continue;

    fallback_font_idxs.emplace(unicode, font_idx);
    return font;
  }

  _missing_glyphs[key].emplace(unicode);
  return {};
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

auto TextEngine::find_notdef_glyph_font() noexcept -> Font*
{
  if (_notdef_font) return _notdef_font;

  err_if(true, "there are not have any font be loaded!");
  std::unreachable();
}

void TextEngine::add_uncached_glyph(Font* font, uint glyph_idx, GlyphKey key, ParseResult& result) noexcept
{
  assert(font);
  if (!_glyph_infos.contains(key))
  {
    result.generating_glyph_info_keys.emplace(key);
    if (_uncached_glyphs.contains(key)) return;
    _uncached_glyphs.emplace(key);
    _ungenerated_glyphs.emplace(key);
  }
}

auto TextEngine::add_notdef_glyph() noexcept -> bool
{
  auto font = find_notdef_glyph_font();

  auto k = GlyphKey{ font->id(), 0 };
  if (_glyph_infos.contains(k)) return false;
  if (_uncached_glyphs.contains(k)) return true;

  _uncached_glyphs.emplace(k);
  _ungenerated_glyphs.emplace(k);
  return true;
}

auto TextEngine::calc_glyph_pos(float2 extent) noexcept -> std::pair<uint, float2>
{
  check(extent.x > Glyph_Atlas_Width || extent.y > Glyph_Atlas_Height,
        "too big glyph sdf bitmap, cannot be stored in glyph atlas");

  static auto current_glyph_atlas_idx{ 0u };
  
  if (auto res = _packer.add(extent.x, extent.y); res)
    return { current_glyph_atlas_idx, res.value() };

  ++current_glyph_atlas_idx;
  _glyph_atlas.emplace_back(g_img_mgr.create(Glyph_Atlas_Width, Glyph_Atlas_Height, ImageFormat::rgba8_unorm, ImageType::srv));

  _packer.reset({ Glyph_Atlas_Width, Glyph_Atlas_Height });
  auto res = _packer.add(extent.x, extent.y);
  assert(res);

  return { current_glyph_atlas_idx, res.value() };
}

void TextEngine::submit_bitmap_generation_tasks() noexcept
{
  if (_ungenerated_glyphs.empty()) return;

  // submit msdf bitmap generate task on thread pool
  _generate_bitmap_tasks.emplace_back(g_thread_pool.submit([glyphs = std::move(_ungenerated_glyphs)]
  {
    auto bitmaps = std::vector<MSDFBitmap>{};
    bitmaps.reserve(glyphs.size());
    for (auto k : glyphs)
      // font should always be valid because I never remove font currently
      bitmaps.emplace_back(g_text_engine._fonts[k.font_id()]->generate_msdf_bitmap(k.codepoint(), k));
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
    if (std::ranges::any_of(result.generating_glyph_info_keys, [&](auto const& k) { return !_glyph_infos.contains(k); }))
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

  reload_missing_glyphs();
}

void TextEngine::reload_missing_glyphs() noexcept
{
  for (auto& [font_style_key, keys] : _pending_reload_missing_glyphs)
  {
    if (keys.empty() || std::ranges::any_of(keys, [&](auto const& key) { return !_glyph_infos.contains(key); }))
      continue;
    remove_missing_glyphs(font_style_key);
    keys.clear();
  }
}

void TextEngine::remove_missing_glyphs(FontStyleKey key) noexcept
{
  // clear missing glyphs and cached text advances
  _missing_glyphs[key].clear();
  for (auto const& key : _cached_texts_with_missing_glyphs[key])
  {
    auto it = _cached_text_parse_results.find(key);
    assert(it != _cached_text_parse_results.end());
    auto h = it->second;
    assert(h.valid());
    if (auto fallback_it = _last_ready_text_parse_results.find(key.text_hash);
        fallback_it != _last_ready_text_parse_results.end() && fallback_it->second == h)
      _last_ready_text_parse_results.erase(fallback_it);
    _discard_text_parse_result_handles.emplace_back(h);
    _cached_text_parse_results.erase(it);
  }
  _cached_texts_with_missing_glyphs[key].clear();
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

void TextEngine::update() noexcept
{
  submit_bitmap_generation_tasks();
  upload_bitmaps();
}

void TextEngine::clear_pending_copy_glyphs() noexcept
{
  // Cache glyphs before clearing
  g_glyph_cacher.add(_pending_copy_glyphs);
  _pending_copy_glyphs.clear();
}

}
