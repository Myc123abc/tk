#pragma once

#include "font_selector.hpp"

inline auto font_selector = FontSelector{};

inline std::string_view startup_font_paths[]
{
  "assets/font/NotoSansJP-Regular.ttf",
  "assets/font/NotoSansJP-Bold.ttf",
  "assets/font/YuGothR.ttc",
  "assets/font/SourceCodePro-Regular.ttf",
};

inline std::string_view fallback_font_paths[]
{
  "assets/font/NotoSansJP-Regular.ttf",
  "assets/font/NotoSansSC-Regular.ttf",
};

inline void font_init() noexcept
{
  for (auto path : startup_font_paths)
    font_selector.add(path);
  font_selector.load_fonts();
}
