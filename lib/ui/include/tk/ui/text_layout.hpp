#pragma once

#include "ui.hpp"

#include <span>

namespace tk::ui {

class TextLayout
{
public:
  enum class Direction
  {
    horizontal,
    vertical,
  };

  enum class TextOrder
  {
    forward,
    reverse,
  };

  TextLayout() = default;

  TextLayout(std::span<std::string const> texts, std::string_view family = {}, FontStyle style = {}, TextDirection direction = {}) noexcept;

  auto adjust_size(float size) noexcept -> TextLayout&;

  auto center_alignment(Direction direction = {}, TextOrder order = {}) noexcept -> TextLayout&;

  void ignore_text(uint idx) noexcept { _texts.at(idx).ignore = true; }
  auto adjust_ratios(std::span<float> ratios) noexcept -> TextLayout&;

  auto set_pos(float2 pos)          noexcept -> TextLayout& { _pos           = pos;      return *this; }
  auto set_pos(float x, float y)    noexcept -> TextLayout& { _pos           = { x, y }; return *this; }
  auto set_padding(float2 padding)  noexcept -> TextLayout& { _padding       = padding;  return *this; }
  auto set_padding_x(float padding) noexcept -> TextLayout& { _padding.x     = padding;  return *this; }
  auto set_padding_y(float padding) noexcept -> TextLayout& { _padding.y     = padding;  return *this; }
  auto set_color(Color color)       noexcept -> TextLayout& { _inner_color   = color;    return *this; }
  auto set_outer_color(Color color) noexcept -> TextLayout& { _outer_color   = color;    return *this; }
  auto set_outline_width(float w)   noexcept -> TextLayout& { _outline_width = w;        return *this; }

  auto& texts()         const noexcept { return _texts;         }
  auto  pos()           const noexcept { return _pos;           }
  auto  extent()        const noexcept { return _extent;        }
  auto  width()         const noexcept { return _extent.x;      }
  auto  height()        const noexcept { return _extent.y;      }
  auto  scale()         const noexcept { return _scale;         }
  auto  color()         const noexcept { return _inner_color;   }
  auto  outer_color()   const noexcept { return _outer_color;   }
  auto  outline_width() const noexcept { return _outline_width; }

  auto  unit_width()    const noexcept { return _max_width  + _padding.x; }
  auto  unit_height()   const noexcept { return _max_height + _padding.y; }

private:
  struct Text
  {
    std::string_view text;
    float2           pos;
    float2           extent;
    uint64           handle;
    bool             ignore{};
  };
  std::vector<Text> _texts;
  float2            _pos;
  float2            _extent;
  float             _scale{};

  Color             _inner_color;
  Color             _outer_color;
  float             _outline_width{};

  float             _max_width{};
  float             _max_height{};
  float2            _padding;
  Direction         _direction{};
  TextOrder         _text_order{};
};

}
