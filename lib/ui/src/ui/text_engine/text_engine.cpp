#include "text_engine.hpp"
#include "vertical_orientation.hpp"

#include <ranges>

namespace tk::ui {

auto TextEngine::uses_vertical_shaping(uint codepoint) noexcept -> bool
{
  return tk::ui::uses_vertical_shaping(codepoint);
}

void TextEngine::init() noexcept
{
  _shaper.init();
  _glyph_store.init();
}

void TextEngine::destroy() noexcept
{
  _glyph_store.destroy();
  _font_registry.clear();
  for (auto& handle : _cached_text_parse_results | std::views::values)
  {
    assert(handle.valid());
    _parse_result_pool.free(handle);
  }
  _shaper.destroy();
}

auto TextEngine::load_font(std::string const& path) noexcept -> std::expected<FontInfo, FontLoadErrorType>
{
  auto result = _font_registry.load_font(path);
  queue_pending_missing_glyphs();
  return result;
}

auto TextEngine::load_fonts(std::span<std::string const> paths) noexcept -> std::vector<std::expected<FontInfo, FontLoadErrorType>>
{
  auto results = _font_registry.load_fonts(paths);
  queue_pending_missing_glyphs();
  return results;
}

void TextEngine::queue_pending_missing_glyphs() noexcept
{
  for (auto const& keys : _font_registry.pending_missing_glyphs() | std::views::values)
    for (auto const& key : keys)
      _glyph_store.request(key);
}

auto TextEngine::try_to_get_parse_result(uint64 text_id, uint64 fallback_id) noexcept -> std::optional<TextParseResultHandle>
{
  if (auto it = _cached_text_parse_results.find(text_id); it != _cached_text_parse_results.end())
  {
    auto const& result = _parse_result_pool[it->second];
    if (result.generate_complete())
    {
      _last_ready_text_parse_results[fallback_id] = it->second;
      return it->second;
    }
    if (auto fallback_it = _last_ready_text_parse_results.find(fallback_id); fallback_it != _last_ready_text_parse_results.end())
      return fallback_it->second;
    return it->second;
  }
  return {};
}

auto TextEngine::parse(StringLiteral text, std::string_view family, FontStyle style, TextDirection direction, float tracking, float limit_len) noexcept -> TextParseResultHandle
{
  assert(!text.empty());

  auto id = get_text_id(text, family, style, direction, tracking, limit_len);
  // Pending layouts may reuse a previous result, but never another direction.
  auto const fallback_id = hash(text.id(), direction);
  if (auto result = try_to_get_parse_result(id, fallback_id)) return *result;

  auto result          = ParseResult{ text.view(), direction };
  auto glyph_style_key = FontStyleKey(family, style);
  // Resolve by grapheme cluster so ZWJ emoji and variation sequences never
  // split across fonts before HarfBuzz sees them.
  auto resolved        = _font_registry.resolve(text.view(), glyph_style_key);
  for (auto const& run : resolved.runs)
  {
    auto const cluster_offset = static_cast<uint>(run.text.data() - text.view());
    auto font = run.font ? run.font : _font_registry.notdef_font();

    auto const notdef        = !run.font;
    auto const shaped_glyphs = notdef
      ? _shaper.shape_notdef(run.text, font, direction, resolved.metrics)
      : _shaper.shape(run.text, font, direction, resolved.metrics);
    for (auto const& glyph : shaped_glyphs)
    {
      auto const glyph_index = notdef ? 0u : glyph.glyph_index;
      auto const key         = GlyphKey{ font->key(), glyph_index };
      result.emplace_back(glyph, cluster_offset, key);
      if (_glyph_store.request(key))
        result.generating_glyph_info_keys.emplace(key);
    }
  }

  if (resolved.has_missing_glyphs)
    _cached_texts_with_missing_glyphs[glyph_style_key].emplace_back(id, fallback_id);

  if (direction == TextDirection::horizontal)
  {
    result.extent.y = resolved.metrics.height;
    result.ascender = resolved.metrics.ascender;
  }
  else
  {
    result.extent.x = resolved.metrics.height;
    result.ascender = result.extent.x * .5f;
    for (auto& offset : result.offsets)
      offset.x += result.ascender;
  }

  auto is_generate_complete = result.generate_complete();
  auto handle               = _parse_result_pool.alloc();
  if (direction == TextDirection::horizontal)
  {
    result.apply_tracking(tracking);
    result.limit_length(text.view(), limit_len);
  }
  else if (is_generate_complete)
  {
    result.prepare_layout(_glyph_store.infos());
    result.apply_tracking(tracking);
    result.limit_length(text.view(), limit_len);
  }
  else if (tracking != 0.f || limit_len > 0.f)
  {
    // Visible ink metrics are unavailable until bitmap generation completes.
    // Keep shaped advances so vertical preparation can be replayed before
    // tracking and wrapping rather than mutating an already wrapped result.
    _deferred_vertical_layouts.emplace(handle, DeferredVerticalLayout{ result.advances, result.extent, std::string{ text.view() }, tracking, limit_len });
    result.apply_tracking(tracking);
    result.limit_length(text.view(), limit_len);
  }

  auto [_, inserted] = _cached_text_parse_results.emplace(id, handle);
  assert(inserted);

  if (is_generate_complete)
    result.finalize(_glyph_store.infos());
  else
    _generating_results.emplace(handle);

  _parse_result_pool[handle] = std::move(result);

  if (is_generate_complete)
    _last_ready_text_parse_results[fallback_id] = handle;
  else
    if (auto fallback_it = _last_ready_text_parse_results.find(fallback_id); fallback_it != _last_ready_text_parse_results.end())
      return fallback_it->second;

  return handle;
}

void TextEngine::reload_missing_glyphs() noexcept
{
  for (auto const& [font_style_key, keys] : _font_registry.pending_missing_glyphs())
  {
    if (keys.empty() || std::ranges::any_of(keys, [&](auto const& key) { return !_glyph_store.contains(key); }))
      continue;
    remove_missing_glyphs(font_style_key);
  }
}

void TextEngine::remove_missing_glyphs(FontStyleKey key) noexcept
{
  for (auto [id, fallback_id] : _cached_texts_with_missing_glyphs[key])
  {
    auto it = _cached_text_parse_results.find(id);
    assert(it != _cached_text_parse_results.end());
    auto handle = it->second;
    assert(handle.valid());
    if (auto fallback_it = _last_ready_text_parse_results.find(fallback_id);
        fallback_it != _last_ready_text_parse_results.end() && fallback_it->second == handle)
      _last_ready_text_parse_results.erase(fallback_it);
    _discard_text_parse_result_handles.emplace_back(handle);
    _cached_text_parse_results.erase(it);
  }
  _cached_texts_with_missing_glyphs[key].clear();
  _font_registry.complete_missing_glyph_reload(key);
}

void TextEngine::update() noexcept
{
  if (!_glyph_store.update(_font_registry))
    return;

  for (auto it = _generating_results.begin(); it != _generating_results.end();)
  {
    auto& result = _parse_result_pool[*it];
    assert(!result.generating_glyph_info_keys.empty());
    if (std::ranges::any_of(result.generating_glyph_info_keys,
        [&](auto const& key) { return !_glyph_store.contains(key); }))
    {
      ++it;
      continue;
    }

    result.generating_glyph_info_keys.clear();
    if (result.direction == TextDirection::vertical)
    {
      // Vertical layout depends on actual bitmap ink bounds. Restore the raw
      // shape result and run the same pipeline used by an immediate cache hit.
      auto deferred = _deferred_vertical_layouts.find(*it);
      if (deferred != _deferred_vertical_layouts.end())
      {
        result.advances = std::move(deferred->second.advances);
        result.extent   = deferred->second.extent;
      }
      result.prepare_layout(_glyph_store.infos());
      if (deferred != _deferred_vertical_layouts.end())
      {
        result.apply_tracking(deferred->second.tracking);
        result.limit_length(deferred->second.text, deferred->second.limit_len);
        _deferred_vertical_layouts.erase(deferred);
      }
    }
    result.finalize(_glyph_store.infos());
    it = _generating_results.erase(it);
  }

  reload_missing_glyphs();
}

void TextEngine::clear_pending_copy_glyphs() noexcept
{
  _glyph_store.clear_pending_copy_glyphs();
}

void TextEngine::postprocess() noexcept
{
  for (auto handle : _discard_text_parse_result_handles)
    _parse_result_pool.free(handle);
  _discard_text_parse_result_handles.clear();
}

auto TextEngine::get_bounding_rect(StringLiteral text, std::string_view family, FontStyle style, TextDirection direction, float tracking, float limit_len) noexcept -> std::optional<Rect>
{
  auto handle = parse(text, family, style, direction, tracking, limit_len);
  auto& result = _parse_result_pool[handle];

  if (!result.generate_complete()) return {};
  if (result.bounding_rect)        return *result.bounding_rect;

  auto rect     = Rect{};
  auto pos      = float2{};
  auto ascender = result.direction == TextDirection::vertical ? 0.f : result.ascender;
  for (auto i = 0uz; i < result.glyph_info_keys.size(); ++i)
  {
    auto const& info = _glyph_store.info(result.glyph_info_keys[i]);
    auto const bounds = glyph_rect(info, pos + result.offsets[i] + float2{ 0.f, ascender },
      is_sideways(result.orientations[i]), false);
    rect.expand(bounds.left_top());
    rect.expand(bounds.right_bottom());
    pos += result.advances[i];
  }

  result.bounding_rect = rect;
  return rect;
}

}
