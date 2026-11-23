module;

#include <expected>
#include <optional>
#include <span>
#include <string>
#include <vector>

module tk.ui;

import :text;
import :ui.util;
import tk.base;
import tk.rect;
import tk.string_id;
import tk.ui.context;
import tk.ui.text.engine;
import tk.ui.text.ideographs;
import tk.error_handling;
import tk.unicode;

namespace tk::ui {

auto load_font(std::string const& path) noexcept -> std::expected<FontInfo, FontLoadErrorType>
{
  return g_text_engine.load_font(path);
}

auto load_fonts(std::span<std::string const> paths) noexcept -> std::vector<std::expected<FontInfo, FontLoadErrorType>>
{
  return g_text_engine.load_fonts(paths);
}

auto text(StringLiteral text, float2 pos, float size, Color color, TextConfig const& cfg) noexcept -> TextResult
{
  if (text.empty()) return {};
  auto const item_pos = pos;
  detail::adjust_pos(pos);
  detail::adjust_scale(size);
  auto result = g_ctx.text(text, pos, size, color, cfg);
  g_ctx.register_item({ item_pos, item_pos + result.extent });
  return result;
}

auto text(StringLiteral text, float size, TextConfig const& cfg) noexcept -> TextResult
{
  if (text.empty()) return {};
  detail::adjust_scale(size);
  auto result = g_ctx.text(text, {}, size, {}, cfg);
  g_ctx.register_item({ float2{}, result.extent });
  return result;
}

auto get_text_bounding_rect(StringLiteral text, float size, TextConfig const& cfg) noexcept -> std::optional<Rect>
{
  if (text.empty()) return {};
  detail::adjust_scale(size);
  return g_ctx.get_text_bounding_rect(text, size, cfg);
}

void generate_glyphs_cache(std::string_view path, std::string_view chars) noexcept
{
  err_if(chars.empty(), "chars is empty for generating glyphs cache");

  auto font = load_font(std::string{ path });
  err_if(!font, "failed to load font {} for glyph cache generation", path);

  auto text = std::string{ chars };
  // TODO: any better way?
  (void)g_text_engine.parse(StringLiteral{ text }, font->family, font->style, TextDirection::horizontal, 0.f, 0.f);
}

void generate_glyphs_cache(std::string_view path, std::span<uint8> chars) noexcept
{
  generate_glyphs_cache(path, std::string{ chars.begin(), chars.end() });
}

void generate_glyphs_cache(std::string_view path, std::span<Language> langs) noexcept
{
  auto chars = std::string{};

  for (auto lang : langs)
  {
    switch (lang)
    {
    case Language::en:
      for (auto codepoint = 0x20u; codepoint <= 0xFFu; ++codepoint)
        append_codepoint(chars, codepoint);
    break;

    case Language::zh_cn:
      chars.append(Chinese_Common_Ideographs);
    break;

    case Language::jp:
      chars.append(Japanese_Common_Ideographs);
    break;

    case Language::ko:
      for (auto codepoint = 0x3131u; codepoint <= 0x3163u; ++codepoint)
        append_codepoint(chars, codepoint);
      chars.append(Korean_Common_Syllables);
    break;

    case Language::zh_tw:
      chars.append(Traditional_Chinese_Common_Ideographs);
    break;

    case Language::ru:
      append_codepoint(chars, 0x0401u);
      for (auto codepoint = 0x0410u; codepoint <= 0x044Fu; ++codepoint)
        append_codepoint(chars, codepoint);
      append_codepoint(chars, 0x0451u);
    break;

    default:
      err_if(true, "unsupport language for generating glyphs cache");
    }
  }
  generate_glyphs_cache(path, chars);
}

}
