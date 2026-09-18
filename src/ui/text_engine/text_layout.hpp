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
  float             scale;
  Color             inner_color;
  Color             outer_color;
  float             outline_width;
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
  }
  return layout;
}

void test_text_layout() noexcept;

}
