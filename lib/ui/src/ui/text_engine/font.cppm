module;

#include <ft2build.h>
#include FT_FREETYPE_H
#include <hb.h>

#include <mutex>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

export module tk.ui.text.font;

import tk.base;
import tk.hash;
import tk.rect;
import tk.renderer;
import tk.string_id;
import tk.ui;
import tk.ui.color;
import tk.ui.text.types;

export namespace tk::ui {

struct FontStyleKey
{
  uint64    family_hash{};
  FontStyle style{};

  FontStyleKey() = default;
  FontStyleKey(StringID family, FontStyle style) noexcept
    : family_hash(family), style(style) {}

  auto operator==(FontStyleKey const&) const noexcept -> bool = default;
};

struct FontStyleKeyHash
{
  auto operator()(FontStyleKey const& key) const noexcept
  {
    return hash(key.family_hash, key.style);
  }
};

template <typename T>
using FontStyleMap = std::unordered_map<FontStyleKey, T, FontStyleKeyHash>;

struct GlyphKey
{
  GlyphKey() noexcept = default;
  GlyphKey(FontStyleKey const& font_key, uint glyph_index) noexcept
    : font_key(font_key), glyph_index(glyph_index) {}

  auto operator==(GlyphKey const&) const noexcept -> bool = default;

  FontStyleKey font_key;
  uint         glyph_index{};
};

struct GlyphKeyHash
{
  auto operator()(GlyphKey const& key) const noexcept
  {
    return hash(FontStyleKeyHash{}(key.font_key), key.glyph_index);
  }
};

template <typename T>
using GlyphKeyMap = std::unordered_map<GlyphKey, T, GlyphKeyHash>;
using GlyphKeySet = std::unordered_set<GlyphKey, GlyphKeyHash>;

struct MSDFBitmap
{
  std::vector<uint8> data;
  uint2              extent{};
  float2             pos_offset{};
  float2             visible_offset{};
  float2             visible_extent{};
  GlyphKey           glyph_key;
  bool               is_color{};

  auto to_bitmap_view() const noexcept -> renderer::BitmapView
  {
    return { data.data(), extent.x, extent.y, extent.x * 4 };
  }

  auto empty() const noexcept { return !extent.x || !extent.y; }
};

using PendingCopyGlyphsInfoType = std::unordered_map<uint, std::vector<std::pair<MSDFBitmap, float2>>>;

class Font
{
  friend class TextShaper;
public:
  ~Font() noexcept;

  auto init(std::string const& path) noexcept -> uint8;

  auto name()  const noexcept { return _name;  }
  auto style() const noexcept { return _style; }
  auto family() const noexcept { return _family; }
  auto key() const noexcept { return _key; }
  auto ascender() const noexcept { return _ascender; }
  auto height() const noexcept { return _height; }
  auto has_color_glyphs() const noexcept
  {
    std::lock_guard lock{ _mutex };
    return FT_HAS_COLOR(_face);
  }

  auto find_glyph(uint unicode) const noexcept
  {
    std::lock_guard lock{ _mutex };
    return FT_Get_Char_Index(_face, unicode);
  }

  auto get_glyph_advance(uint glyph_idx) const noexcept -> float2;
  auto get_glyph_bounds(uint glyph_idx) const noexcept -> Rect;
  auto generate_bitmap(uint glyph_idx, GlyphKey const& key) const noexcept -> MSDFBitmap;
  auto generate_msdf_bitmap(uint glyph_idx, GlyphKey const& key) const noexcept -> MSDFBitmap;

private:
  std::string  _name;
  std::string  _family;
  FontStyle    _style;
  FT_Library   _ft{};
  FT_Face      _face{};
  hb_font_t*   _hb_font{};
  float        _ascender{};
  float        _height{};
  FontStyleKey _key;

  mutable std::mutex _mutex;
};

class TextShaper
{
public:
  void init() noexcept;
  void destroy() noexcept;

  auto shape(std::string_view text, Font* font, TextDirection direction, TextLineMetrics metrics) noexcept -> std::vector<ShapedGlyph>;
  auto shape_notdef(std::string_view text, Font* font, TextDirection direction, TextLineMetrics metrics) noexcept -> std::vector<ShapedGlyph>;

private:
  auto shape_text(std::string_view text, Font* font, TextDirection direction, ShapingMode mode, TextLineMetrics metrics) noexcept -> std::vector<ShapedGlyph>;

  hb_buffer_t* _buffer{};
};

}
