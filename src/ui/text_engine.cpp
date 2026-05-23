#include "text_engine.hpp"
#include "util/error_handling.hpp"
#include "image_manager.hpp"

#include <hb-ft.h>
#include <utf8.h>

#include <ranges>

#define check(x, msg) err_if(static_cast<bool>(x), "[TextEngine] {}", msg)

using namespace tk::renderer;

namespace tk::ui {

void Font::init(std::string_view path) noexcept
{
  _name = path;

  check(FT_New_Face(g_text_engine._ft, path.data(), 0, &_face), "failed to load font");
  // TODO: small pixel size will lead complex glyph generate sdf bitmap not right when render in big size
  //       try to use dynamic pixel size adjust by complexity of glyph
  //       and store in different glyph size altas to save memory
  static constexpr auto Pixel_Size = 32;
  check(FT_Set_Pixel_Sizes(_face, 0, Pixel_Size), "failed to set pixel size");

  _hb_font = hb_ft_font_create(_face, nullptr);

  _ascender = static_cast<float>(_face->ascender) * Pixel_Size / _face->units_per_EM;
  _height   = static_cast<float>(_face->height)   * Pixel_Size / _face->units_per_EM;

  using enum FontStyle;
  switch (_face->style_flags)
  {
  case FT_STYLE_FLAG_ITALIC:
    _style = italic;
    break;
  case FT_STYLE_FLAG_BOLD:
    _style = bold;
    break;
  case FT_STYLE_FLAG_ITALIC | FT_STYLE_FLAG_BOLD:
    _style = italic_bold;
    break;
  default:
    _style = regular;
    break;
  }
}

void Font::destroy() const noexcept
{
  hb_font_destroy(_hb_font);
  check(FT_Done_Face(_face), "failed to destroy font");
}

void TextEngine::init() noexcept
{
  check(FT_Init_FreeType(&_ft), "failed to initialize freetype");
  _hb_buf = hb_buffer_create();

  // create the first glyph atlas
  _glyph_atlas.emplace(g_img_mgr.create_image(Glyph_Atlas_Width, Glyph_Atlas_Height, ImageFormat::r8_unorm));
}

void TextEngine::destroy() const noexcept
{
  for (auto handle : _glyph_atlas) g_img_mgr.destroy_image(handle);
  for (auto const& font : _fonts | std::views::values | std::views::join) font.destroy();

  hb_buffer_destroy(_hb_buf);
  check(FT_Done_FreeType(_ft), "failed to destroy freetype");
}

void TextEngine::load_font(std::string_view path) noexcept
{
  if (std::ranges::any_of(_fonts | std::views::values | std::views::join, [&](auto const& font) { return font.name() == path; }))
  {
    warn("[TextEngine] font {} is already loaded", path);
    return;
  }
  
  // create font
  auto font = Font{};
  font.init(path);
  _fonts[font.style()].emplace_back(std::move(font));

  // TODO: clear missing glyphs so can reload them,
  //       or check which glyphs can be loaded in here?
  // TODO: clear cached text advances, why? because some has missing glyphs?
}

auto TextEngine::parse(std::string_view text) noexcept -> ParseResult
{
  auto res = ParseResult{};

  auto u32str = utf8::utf8to32(text);

  // TODO: try to get cached ParseResult

  // calculate advances
  _advances.reserve(u32str.size());

  return res;
}

}
