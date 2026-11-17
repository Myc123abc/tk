module;

extern "C"
{
#include <graphemebreak.h>
}

#include <cassert>
#include <string_view>
#include <vector>

export module tk.ui.text.grapheme_iterator;

import tk.base;
import tk.unicode;

export namespace tk {

// Positions are UTF-8 byte offsets with exclusive range ends. The source text
// must outlive this helper and any views returned by grapheme().
class GraphemeIterator
{
public:
  GraphemeIterator(std::string_view text) noexcept
    : _text(text), _grapheme_breaks(text.size())
  {
    assert(!text.empty());
    set_graphemebreaks_utf8(reinterpret_cast<utf8_t const*>(text.data()), text.size(), nullptr, _grapheme_breaks.data());
  }

  auto begin() const noexcept { return 0u;           }
  auto end()   const noexcept { return _text.size(); }

  auto next(uint beg) const noexcept
  {
    assert(beg < _text.size());
    auto end = beg;
    // libunibreak stores each boundary on the preceding codepoint's last byte.
    do end += next_codepoint(_text, end).size;
    while (end < _text.size() && _grapheme_breaks[end - 1] != GRAPHEMEBREAK_BREAK);
    return end;
  }

  auto grapheme(uint beg, uint end) const noexcept
  {
    assert(beg <= end && end <= _text.size());
    return _text.substr(beg, end - beg);
  }

private:
  std::string_view  _text;
  std::vector<char> _grapheme_breaks;
};

}
