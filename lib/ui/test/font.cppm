module;

#include <array>
#include <string_view>

export module tk.ui.test.font;

import tk.base;
import tk.ui;
import tk.ui.test.font_selector;

using namespace tk;

export inline auto font_selector = FontSelector{};

export inline std::string_view startup_font_paths[]
{
  "assets/font/NotoSansJP-Regular.ttf",
  "assets/font/NotoSansJP-Bold.ttf",
  "assets/font/YuGothR.ttc",
  "assets/font/SourceCodePro-Regular.ttf",
#ifdef _WIN32
  "C:\\Windows\\Fonts\\segoeui.ttf",
  "C:\\Windows\\Fonts\\seguisym.ttf",
  "C:\\Windows\\Fonts\\seguiemj.ttf",
#endif
};

export inline std::string_view fallback_font_paths[]
{
  "assets/font/NotoSansJP-Regular.ttf",
  "assets/font/NotoSansSC-Regular.ttf",
};

export inline void font_init() noexcept
{
  for (auto path : startup_font_paths)
    font_selector.add(path);
  font_selector.load_fonts();

  auto const jp_languages = std::array{ ui::Language::en, ui::Language::jp };
  ui::generate_glyphs_cache(fallback_font_paths[0], jp_languages);

  auto const zh_cn_languages = std::array{ ui::Language::zh_cn };
  ui::generate_glyphs_cache(fallback_font_paths[1], zh_cn_languages);
}
