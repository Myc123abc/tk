#include "text_engine.hpp"
#include "util/error_handling.hpp"

#define check(x, msg) err_if(static_cast<bool>(x), "[TextEngine] {}", msg)

namespace tk { namespace ui {

void TextEngine::init() noexcept
{
  check(FT_Init_FreeType(&_ft), "failed to initialize freetype");
  _hb_buf = hb_buffer_create();
}

void TextEngine::destroy() const noexcept
{
  hb_buffer_destroy(_hb_buf);
  check(FT_Done_FreeType(_ft), "failed to destroy freetype");
}

auto TextEngine::parse(std::string_view text) noexcept -> ParseResult
{
  auto res = ParseResult{};

  return res;
}

}}
