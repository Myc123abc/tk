#pragma once

#include "text_engine.hpp"

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

  void set_pos(float2 pos)          noexcept { _pos           = pos;     }
  void set_padding(float2 padding)  noexcept { _padding       = padding; }
  void set_padding_x(float padding) noexcept { _padding.x     = padding; }
  void set_padding_y(float padding) noexcept { _padding.y     = padding; }
  void set_color(Color color)       noexcept { _inner_color   = color;   }
  void set_outer_color(Color color) noexcept { _outer_color   = color;   }
  void set_outline_width(float w)   noexcept { _outline_width = w;       }

  auto& texts()         const noexcept { return _texts;         }
  auto  pos()           const noexcept { return _pos;           }
  auto  extent()        const noexcept { return _extent;        }
  auto  scale()         const noexcept { return _scale;         }
  auto  color()         const noexcept { return _inner_color;   }
  auto  outer_color()   const noexcept { return _outer_color;   }
  auto  outline_width() const noexcept { return _outline_width; }

private:
  struct Text
  {
    std::string_view      text;
    float2                pos;
    float2                extent;
    TextParseResultHandle handle;
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
};

}
