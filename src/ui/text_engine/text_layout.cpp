#include "text_layout.hpp"

#include <ranges>

namespace tk::ui {

TextLayout::TextLayout(std::span<std::string const> texts, std::string_view family, FontStyle style, TextDirection direction) noexcept
{
  _texts.reserve(texts.size());
  for (std::string_view text : texts)
  {
    auto handle = g_text_engine.parse(text, family, style, direction);
    auto extent = g_text_engine.get_parse_result(handle).extent;

    _texts.emplace_back(text, float2{}, extent, handle);

    _max_width  = std::max(_max_width,  extent.x);
    _max_height = std::max(_max_height, extent.y);
  }
}

auto TextLayout::adjust_size(float size) noexcept -> TextLayout&
{
  _scale = size / FT_Pixel_Size;
  for (auto& text : _texts) text.extent *= _scale;
  _max_width  *= _scale;
  _max_height *= _scale;
  _extent     *= _scale;
  return *this;
}

auto TextLayout::center_alignment(Direction direction, TextOrder order) noexcept -> TextLayout&
{
  auto offset = 0.f;
  auto width  = unit_width();
  auto height = unit_height();

  auto align_text = [&](Text& text) noexcept
  {
    if (direction == Direction::horizontal)
    {
      text.pos.x = offset + (width - text.extent.x) / 2;
      text.pos.y = (height - text.extent.y) / 2;
      offset += width;
    }
    else
    {
      text.pos.x = (width - text.extent.x) / 2;
      text.pos.y = offset + (height - text.extent.y) / 2;
      offset += height;
    }
  };

  if (order == TextOrder::reverse)
    for (auto& text : std::views::reverse(_texts)) align_text(text);
  else
    for (auto& text : _texts) align_text(text);

  if (direction == Direction::horizontal)
    _extent = { offset, height };
  else
    _extent = { width, offset };

  return *this;
}

}
