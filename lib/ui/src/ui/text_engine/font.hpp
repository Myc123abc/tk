#pragma once

#include "ui/ui.hpp"
#include "../../util/hash.hpp"

#include <ft2build.h>
#include FT_FREETYPE_H
#include <hb.h>

#include <string>
#include <mutex>
#include <unordered_map>

namespace tk::ui {

struct FontStyleKey
{
  size_t    family_hash{};
  FontStyle style{};

  FontStyleKey() = default;
  FontStyleKey(std::string_view family, FontStyle style) noexcept
    : family_hash(generic_hash(family)), style(style) {}

  auto operator==(FontStyleKey const&) const noexcept -> bool = default;
};

struct FontStyleKeyHash
{
  auto operator()(FontStyleKey const& key) const noexcept
  {
    return generic_hash(key.family_hash, key.style);
  }
};

template <typename T>
using FontStyleMap = std::unordered_map<FontStyleKey, T, FontStyleKeyHash>;

struct GlyphKey;
struct MSDFBitmap;

class Font
{
  friend class TextEngine;
public:
  auto init(std::string_view path) noexcept -> uint8;
  void destroy() const noexcept;

  auto name()  const noexcept { return _name;  }
  auto style() const noexcept { return _style; }
  auto family() const noexcept { return _family; }
  auto key() const noexcept { return _key; }

  auto find_glyph(uint unicode) const noexcept
  {
    std::lock_guard lock{ _mutex };
    return FT_Get_Char_Index(_face, unicode);
  }

  auto get_glyph_advance(uint glyph_idx) const noexcept -> float2;
  auto generate_msdf_bitmap(uint glyph_idx, GlyphKey const& key) const noexcept -> MSDFBitmap;

private:
  std::string  _name;
  std::string  _family;
  FontStyle    _style;
  FT_Face      _face{};
  hb_font_t*   _hb_font{};
  float        _ascender{};
  float        _height{};
  FontStyleKey _key;

  mutable std::mutex _mutex;
};

}
