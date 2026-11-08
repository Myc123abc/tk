#pragma once

#include "tk/ui/ui.hpp"
#include "tk/ui/text_layout.hpp"
#include "tk/log.hpp"

#include <ranges>
#include <deque>

using namespace tk;

inline auto style_str(ui::FontStyle style) noexcept
{
  switch (style)
  {
  case ui::FontStyle::regular:     return "regular";
  case ui::FontStyle::italic:      return "italic";
  case ui::FontStyle::bold:        return "bold";
  case ui::FontStyle::italic_bold: return "italic bold";
  }
  std::unreachable();
}

class FontSelector
{
public:
  FontSelector()                               = default;
  ~FontSelector()                              = default;
  FontSelector(FontSelector const&)            = delete;
  FontSelector(FontSelector&&)                 = delete;
  FontSelector& operator=(FontSelector const&) = delete;
  FontSelector& operator=(FontSelector&&)      = delete;

  void add(std::string_view& path) noexcept
  {
    if (std::ranges::find(_paths, path)       != _paths.end() ||
        std::ranges::find(_added_paths, path) != _added_paths.end())
      return;
    _added_paths.emplace_back(path);
  }

  void load_fonts() noexcept
  {
    if (_added_paths.empty()) return;
    _font_infos.reserve(_font_infos.size() + _added_paths.size());
    _font_names.resize(_font_infos.size());
    _font_name_views.reserve(_font_infos.size());
    for (auto [res, path] : std::views::zip(ui::load_fonts(_added_paths), _added_paths))
    {
      if (res)
      {
        _font_infos.emplace_back(res.value());
        _font_name_views.emplace_back(
          _font_names.emplace_back(res->family + " " + style_str(res->style)));
      }
      else
        res.error().visit
        (
          [&](ui::FontLoadError::unexist)          { warn("[FontSelector] font {} unexist.", path); },
          [&](ui::FontLoadError::freetype_err err) { warn("[FontSelector] load font {} freetype error : {}.", path, err.code); }
        );
    }
    _paths.append_range(_added_paths);
    _added_paths.clear();
  }

  auto& font_infos() const noexcept { return _font_infos; }

  auto operator()(float2 pos, float size, float2 padding) noexcept -> float2
  {
    _is_selected = false;

    _txt_layout = ui::TextLayout{ _font_name_views }
      .set_pos(pos)
      .adjust_size(size)
      .set_color(0x000000ff)
      .set_padding(padding)
      .alignment(ui::TextLayout::Alignment::left, ui::TextLayout::Direction::vertical);

    auto width  = _txt_layout.extent().x;
    auto height = _txt_layout.unit_height();
    for (auto i = 0; i < _txt_layout.texts().size(); ++i)
    {
      if (ui::button("FontSelector" + std::string(_txt_layout.texts()[i].text), pos.x, pos.y, width, height, 0x00ff00ff, 0x0000ffff))
      {
        _is_selected  = true;
        _selected_idx =  i;
      }
      pos.y += height;
    }
    
    _txt_layout.render();
    return _txt_layout.extent();
  }

  auto is_selected() const noexcept { return _is_selected; }
  auto& selected_font() const noexcept { return _font_infos[_selected_idx]; }

private:
  std::vector<std::string>   _added_paths;
  std::vector<std::string>   _paths;
  std::vector<ui::FontInfo>  _font_infos;
  std::deque<std::string>    _font_names;
  std::vector<StringLiteral> _font_name_views;
  ui::TextLayout             _txt_layout;
  bool                       _is_selected{};
  int                        _selected_idx{ -1 };
};
