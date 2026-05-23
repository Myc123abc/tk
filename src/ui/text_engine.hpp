#pragma once

#include "ui/ui.hpp"
#include "image_manager.hpp"

#include <string>
#include <unordered_set>
#include <unordered_map>

#include "util/base.hpp"

#include <ft2build.h>
#include FT_FREETYPE_H
#include <hb.h>

namespace tk::ui {

class Font
{
public:
  void init(std::string_view path) noexcept;
  void destroy() const noexcept;

  auto name()  const noexcept { return _name;  }
  auto style() const noexcept { return _style; }

private:
  std::string _name;
  FontStyle   _style;
  FT_Face     _face{};
  hb_font_t*  _hb_font{};
  float       _ascender{};
  float       _height{};
};

Singleton(TextEngine, g_text_engine,

  friend class Font;

public:
  void init() noexcept;
  void destroy() const noexcept;

  void load_font(std::string_view path) noexcept;

  struct ParseResult
  {
    float2  extent{};
    uint32_t glyph_atlas_index{};
  };
  auto parse(std::string_view text) noexcept -> ParseResult;

private:
  FT_Library   _ft{};
  hb_buffer_t* _hb_buf{};

  std::unordered_set<ImageHandle>                  _glyph_atlas;
  std::unordered_map<FontStyle, std::vector<Font>> _fonts;

  std::vector<float2> _advances;
)

}
