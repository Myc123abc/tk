#include "font_registry.hpp"
#include "grapheme_iterator.hpp"
#include "tk/error_handling.hpp"
#include "../../util/file_system.hpp"
#include "../../util/async/thread_pool.hpp"

extern "C"
{
#include <emojidef.h>
}

#include <ranges>

namespace tk::ui {

auto FontRegistry::load_font(std::string const& path) noexcept -> std::expected<FontInfo, FontLoadErrorType>
{
  if (auto it = std::ranges::find_if(_fonts, [&](auto const& font) { return font->name() == path; });
      it != _fonts.end())
    return FontInfo{ (*it)->family(), (*it)->style() };

  if (!g_file_sys.exists(path)) return std::unexpected(FontLoadError::unexist{});

  auto font = std::make_unique<Font>();
  if (auto res = font->init(path); res) return std::unexpected(FontLoadError::freetype_err{ res });

  return install_font(std::move(font));
}

auto FontRegistry::load_fonts(std::span<std::string const> paths) noexcept -> std::vector<std::expected<FontInfo, FontLoadErrorType>>
{
  using Result     = std::expected<FontInfo, FontLoadErrorType>;
  using TaskResult = std::pair<std::unique_ptr<Font>, uint8>;

  auto results      = std::vector<std::optional<Result>>(paths.size());
  auto task_indices = std::vector<std::optional<size_t>>(paths.size());
  auto tasks        = std::vector<Task<TaskResult>>{};
  auto pending      = std::unordered_map<std::string, size_t>{};
  tasks.reserve(paths.size());
  pending.reserve(paths.size());

  for (auto i = 0uz; i < paths.size(); ++i)
  {
    auto const path = paths[i];
    if (auto it = std::ranges::find_if(_fonts, [&](auto const& font) { return font->name() == path; });
        it != _fonts.end())
    {
      results[i].emplace(FontInfo{ (*it)->family(), (*it)->style() });
      continue;
    }

    if (!g_file_sys.exists(path))
    {
      results[i].emplace(std::unexpected(FontLoadErrorType{ FontLoadError::unexist{} }));
      continue;
    }

    auto owned_path = std::string{ path };
    auto [it, inserted] = pending.try_emplace(owned_path, tasks.size());
    task_indices[i] = it->second;
    if (!inserted) continue;

    tasks.emplace_back(g_thread_pool.run([path = std::move(owned_path)]() mutable
    {
      auto font = std::make_unique<Font>();
      auto const error = font->init(path);
      return TaskResult{ std::move(font), error };
    }));
  }

  auto task_results = std::vector<Result>{};
  task_results.reserve(tasks.size());
  for (auto& task : tasks)
  {
    auto [font, error] = task.take_result();
    if (error)
      task_results.emplace_back(std::unexpected(FontLoadErrorType{ FontLoadError::freetype_err{ error } }));
    else
      task_results.emplace_back(install_font(std::move(font)));
  }

  auto ordered_results = std::vector<Result>{};
  ordered_results.reserve(paths.size());
  for (auto i = 0uz; i < paths.size(); ++i)
  {
    if (results[i])
      ordered_results.emplace_back(std::move(*results[i]));
    else
      ordered_results.emplace_back(task_results[*task_indices[i]]);
  }
  return ordered_results;
}

auto FontRegistry::install_font(std::unique_ptr<Font> font) noexcept -> FontInfo
{
  auto info = FontInfo{ font->family(), font->style() };

  auto font_idx    = static_cast<uint>(_fonts.size());
  auto loaded_font = _fonts.emplace_back(std::move(font)).get();
  auto res         = _font_idxs.emplace(loaded_font->key(), font_idx);
  assert(res.second);
  _style_font_idxs[static_cast<size_t>(loaded_font->style())].emplace_back(font_idx);

  if (!_notdef_font) _notdef_font = loaded_font;

  for (auto const& [key, missing_glyphs] : _missing_glyphs)
    if (key.style == loaded_font->style())
      regenerate_missing_glyphs(loaded_font, key);

  return info;
}

void FontRegistry::regenerate_missing_glyphs(Font* font, FontStyleKey key) noexcept
{
  auto& missing_glyphs = _missing_glyphs[key];
  if (missing_glyphs.empty()) return;

  auto& glyphs = _pending_missing_glyphs[key];
  for (auto unicode : missing_glyphs)
  {
    auto glyph_idx = font->find_glyph(unicode);
    if (!glyph_idx) continue;

    _fallback_font_idxs[key][unicode] = _font_idxs.at(font->key());
    glyphs.emplace(font->key(), glyph_idx);
  }
}

// split text by grapheme, and record every set of graphemes with supported font.
auto FontRegistry::resolve(std::string_view text, FontStyleKey key) noexcept -> ResolvedText
{
  auto result = ResolvedText{};
  result.runs.reserve(text.size());

  auto max_descender  = 0.f;
  auto update_metrics = [&](Font* font)
  {
    if (!font) font = notdef_font();
    result.metrics.ascender = std::max(result.metrics.ascender, font->_ascender);
    max_descender           = std::max(max_descender, font->_height - font->_ascender);
    result.metrics.height   = result.metrics.ascender + max_descender;
  };
  auto supports_cluster = [](Font const& font, std::string_view cluster)
  {
    for (auto index = 0u; index < cluster.size();)
    {
      auto const [codepoint, size, valid] = next_codepoint(cluster, index);
      assert(valid);
      if (codepoint != 0x200d  &&
        !(codepoint >= 0xfe00  && codepoint <=  0xfe0f) &&
        !(codepoint >= 0xe0100 && codepoint <= 0xe01ef) &&
        !font.find_glyph(codepoint))
        return false;
      index += size;
    }
    return true;
  };
  auto prefers_color_emoji = [](std::string_view cluster)
  {
    // FE0E explicitly requests text presentation. FE0F and extended
    // pictographs prefer a color-capable fallback when one is loaded.
    auto has_emoji = false;
    for (auto index = 0u; index < cluster.size();)
    {
      auto const [codepoint, size, valid] = next_codepoint(cluster, index);
      assert(valid);
      if (codepoint == 0xfe0e) return false;
      has_emoji |= codepoint == 0xfe0f || ub_is_extended_pictographic(codepoint);
      index += size;
    }
    return has_emoji;
  };
  auto find_cluster_font = [&](std::string_view cluster) -> Font*
  {
    if (prefers_color_emoji(cluster))
    {
      // Preserve requested style where possible, then allow any loaded color
      // font so selecting a text-only family does not disable color emoji.
      auto find_color_font = [&](auto&& fonts) -> Font*
      {
        for (auto font_idx : fonts)
        {
          auto font = _fonts[font_idx].get();
          if (font->has_color_glyphs() && supports_cluster(*font, cluster))
            return font;
        }
        return {};
      };

      if (auto it = _font_idxs.find(key); it != _font_idxs.end())
      {
        auto font = _fonts[it->second].get();
        if (font->has_color_glyphs() && supports_cluster(*font, cluster))
          return font;
      }
      if (auto font = find_color_font(_style_font_idxs[static_cast<size_t>(key.style)]))
        return font;
      if (auto font = find_color_font(std::views::iota(0uz, _fonts.size())))
        return font;
    }

    if (auto it = _font_idxs.find(key); it != _font_idxs.end())
    {
      auto font = _fonts[it->second].get();
      if (supports_cluster(*font, cluster)) return font;
    }

    for (auto font_idx : _style_font_idxs[static_cast<size_t>(key.style)])
    {
      auto font = _fonts[font_idx].get();
      if (font->key() != key && supports_cluster(*font, cluster)) return font;
    }
    return {};
  };

  auto grapheme_iterator = GraphemeIterator(text);

  auto find_grapheme_font = [&](uint it)
  {
    auto next_it = grapheme_iterator.next(it);
    auto font    = find_cluster_font(grapheme_iterator.grapheme(it, next_it));
    if (!font) font = find_font(next_codepoint(text, it).codepoint, key);
    result.has_missing_glyphs |= !font;
    return std::pair{ font, next_it };
  };

  auto beg = grapheme_iterator.begin();
  auto [cur_font, it] = find_grapheme_font(beg);
  update_metrics(cur_font);
  while (it < grapheme_iterator.end())
  {
    auto [font, next_it] = find_grapheme_font(it);
    if (font != cur_font)
    {
      result.runs.emplace_back(grapheme_iterator.grapheme(beg, it), cur_font);
      beg      = it;
      cur_font = font;
      update_metrics(cur_font);
    }
    it = next_it;
  }

  result.runs.emplace_back(grapheme_iterator.grapheme(beg, it), cur_font);

  return result;
}

auto FontRegistry::find_font(uint unicode, FontStyleKey key) noexcept -> Font*
{
  // if the unicode is missing, return nullptr
  if (_missing_glyphs[key].contains(unicode)) return {};

  // if not missing, find whether have font support the key with the unicode
  if (auto it = _font_idxs.find(key); it != _font_idxs.end())
  {
    auto font = _fonts[it->second].get();
    if (font->find_glyph(unicode)) return font;
  }

  // if cannot find the prefect font, find the fallback result
  auto& fallback_font_idxs = _fallback_font_idxs[key];
  if (auto fallback_it = fallback_font_idxs.find(unicode); fallback_it != fallback_font_idxs.end())
    return _fonts[fallback_it->second].get();

  // if fallback not have, find the current font style's supporting the unicode font
  for (auto font_idx : _style_font_idxs[static_cast<size_t>(key.style)])
  {
    auto font = _fonts[font_idx].get();
    if (!font->find_glyph(unicode)) continue;

    fallback_font_idxs.emplace(unicode, font_idx);
    return font;
  }

  // if the specific font style no font support, record missing glyph and return nullptr
  _missing_glyphs[key].emplace(unicode);
  return {};
}

auto FontRegistry::notdef_font() noexcept -> Font*
{
  if (_notdef_font) return _notdef_font;

  err_if(true, "there are not have any font be loaded!");
  std::unreachable();
}

auto FontRegistry::font(FontStyleKey key) const noexcept -> Font*
{
  return _fonts[_font_idxs.at(key)].get();
}

void FontRegistry::complete_missing_glyph_reload(FontStyleKey key) noexcept
{
  _missing_glyphs[key].clear();
  _pending_missing_glyphs[key].clear();
}

void FontRegistry::clear() noexcept
{
  _notdef_font = {};
  _fonts.clear();
  _font_idxs.clear();
  for (auto& indices : _style_font_idxs)
    indices.clear();
  _missing_glyphs.clear();
  _fallback_font_idxs.clear();
  _pending_missing_glyphs.clear();
}

}
