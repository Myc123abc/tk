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

}
