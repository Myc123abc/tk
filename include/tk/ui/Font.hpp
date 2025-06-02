//
// Font
//
// use FreeType
//

#pragma once

#include <ft2build.h>
#include FT_FREETYPE_H

#include <string_view>

#include "tk/ErrorHandling.hpp"

namespace tk { namespace ui {

inline auto load_font(std::string_view path)
{
  // init freetype
  FT_Library ft;
  throw_if(FT_Init_FreeType(&ft), "failed to init FreeType Library");

  // load font
  FT_Face face;
  throw_if(FT_New_Face(ft, path.data(), 0, &face), "failed to load font {}", path);

  // set pixel size
  throw_if(FT_Set_Pixel_Sizes(face, 0, 96), "failed to set pixel size in freetype");

  // load character
  char character = 'g';
  throw_if(FT_Load_Char(face, character, FT_LOAD_RENDER), "failed to load char {}", character);

  // destroy resources
  throw_if(FT_Done_Face(face), "failed to unload font");
  throw_if(FT_Done_FreeType(ft), "failed to destroy freetype library");
}


}}