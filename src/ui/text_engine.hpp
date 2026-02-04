#pragma once

#include <string>

#include <glm/glm.hpp>

#include <ft2build.h>
#include FT_FREETYPE_H
#include <hb.h>

namespace tk { namespace ui {

class TextEngine
{
private:
  TextEngine()                             = default;
  ~TextEngine()                            = default;
public:
  TextEngine(TextEngine const&)            = delete;
  TextEngine(TextEngine&&)                 = delete;
  TextEngine& operator=(TextEngine const&) = delete;
  TextEngine& operator=(TextEngine&&)      = delete;

  static auto instance() noexcept -> TextEngine&
  {
    static TextEngine instance;
    return instance;
  }

  void init() noexcept;
  void destroy() const noexcept;

  struct ParseResult
  {
    glm::vec2 extent{};
    uint32_t  glyph_atlas_index{};
  };
  auto parse(std::string_view text) noexcept -> ParseResult;

private:
  FT_Library   _ft{};
  hb_buffer_t* _hb_buf{};
};

inline static auto& g_text_engine{ TextEngine::instance() };

}}
