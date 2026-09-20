#pragma once

#include "text_engine.hpp"

namespace tk::ui {

struct TextLayout
{
  struct Text
  {
    std::string_view      text;
    float2                pos;
    float2                extent;
    TextParseResultHandle handle;
  };
  std::vector<Text> texts;
  float2            pos;
  float2            extent;
  float             scale{};
  Color             inner_color;
  Color             outer_color;
  float             outline_width{};

  float             max_width{};
  float             max_height{};
  float2            padding;

  auto center_aligment() noexcept -> TextLayout&
  {
    auto offset = 0;
    auto width  = max_width + padding.x;
    for (auto& text : texts)
    {
      text.pos.x = offset + (width - text.extent.x) / 2;
      offset += width;
    }
    return *this;
  }
};

inline auto get_text_layout(std::vector<std::string_view> const& texts, std::string_view family, FontStyle style, TextDirection direction) noexcept -> TextLayout
{
  auto layout = TextLayout{};
  layout.texts.reserve(texts.size());
  for (auto text : texts)
  {
    auto handle = g_text_engine.parse(text, family, style, direction);
    auto extent = g_text_engine.get_parse_result(handle).extent;

    layout.texts.emplace_back(text, float2{}, extent, handle);

    layout.max_width  = std::max(layout.max_width,  extent.x);
    layout.max_height = std::max(layout.max_height, extent.y);
  }
  return layout;
}

void test_text_layout() noexcept;

}
