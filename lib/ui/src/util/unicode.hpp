#pragma once

#include "tk/error_handling.hpp"
#include "tk/base.hpp"

namespace tk {

inline auto to_string(std::wstring_view wstr) noexcept
{
  auto size = WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS,
    wstr.data(), wstr.size(), nullptr, 0, nullptr, nullptr);
  err_if(!size, "Failed to convert wstring to string");
  
  auto str = std::string(size, 0);
  WideCharToMultiByte(CP_UTF8, 0,
    wstr.data(), wstr.size(), str.data(), size, nullptr, nullptr);
  return str;
}

inline auto to_wstring(std::string_view str) noexcept
{
  auto size = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS,
    str.data(), str.size(), nullptr, 0);
  err_if(!size, "Failed to convert string to wstring");
  
  auto wstr = std::wstring(size, 0);
  MultiByteToWideChar(CP_UTF8, 0,
    str.data(), str.size(), wstr.data(), size);
  return wstr;
}

inline auto next_codepoint(std::string_view str, uint idx) -> std::pair<uint, uint>
{
  if (idx >= str.size()) return {};

  auto const beg = idx;
  auto const c   = static_cast<uint8>(str[idx]);

  auto res  = 0u;
  auto size = 0u;

  if (c <= 0x7f)
  {
    res  = c;
    size = 1;
  }
  else if ((c & 0xe0) == 0xc0)
  {
    res  = c & 0x1f;
    size = 2;
  }
  else if ((c & 0xf0) == 0xe0)
  {
    res  = c & 0x0f;
    size = 3;
  }
  else if ((c & 0xf8) == 0xf0)
  {
    res  = c & 0x07;
    size = 4;
  }
  else
    // Skip invalid byte
    return { 0xfffd, 1 };

  // Truncated sequence
  if (idx + size > str.size())
    return { 0xfffd, str.size() - idx };

  // Validate continuation bytes
  for (auto i = 1; i < size; ++i)
  {
    auto const byte = static_cast<uint8>(str[idx + i]);
    if ((byte & 0xc0) != 0x80)
      return { 0xfffd, 1 };

    res = (res << 6) | (byte & 0x3f);
  }

  // Reject overlong encoding
  if ((size == 2 && res < 0x80)  ||
      (size == 3 && res < 0x800) ||
      (size == 4 && res < 0x10000))
    return { 0xfffd, 1 };

  // Reject utf16 surrogate range
  if (res >= 0xd800 && res <= 0xdfff)
    return { 0xfffd, 1 };

  // Reject values outside unicode
  if (res > 0x10ffff)
    return { 0xfffd, 1 };

  return { res, size };
}

inline auto codepoint_cnt(std::string_view str) -> uint
{
  auto idx = 0u;
  auto cnt = 0u;
  while (idx < str.size())
  {
    auto const [res, size] = next_codepoint(str, idx);
    err_if(res == 0xfffd, "Invalid u8string: {}", str);
    idx += size;
    ++cnt;
  }
  return cnt;
}

class UTF8Iterator
{
  friend class UTF8View;
public:
  using value_type        = uint;
  using difference_type   = std::ptrdiff_t;
  using iterator_category = std::forward_iterator_tag;

  UTF8Iterator(std::string_view str, uint idx) noexcept
    : _str(str.data()), _size(str.size()), _idx(idx) {}

  auto operator*() const noexcept -> uint
  {
    return next_codepoint({ _str, _size }, _idx).first;
  }

  auto operator++() noexcept -> UTF8Iterator&
  {
    _idx += next_codepoint({ _str, _size }, _idx).second;
    return *this;
  }

  auto operator==(UTF8Iterator const& rhs) const noexcept -> bool
  {
    return _idx == rhs._idx;
  }

  auto operator!=(UTF8Iterator const& rhs) const noexcept -> bool
  {
    return _idx != rhs._idx;
  }

private:
  char const* _str;
  uint        _size{};
  uint        _idx{};
};

inline auto operator+(UTF8Iterator it, uint n) -> UTF8Iterator
{
  while (n--) ++it;
  return it;
}

class UTF8View
{
public:
  using iterator = UTF8Iterator;

  UTF8View(std::string_view view) noexcept
    : _view(view) {}

  auto begin() const noexcept -> iterator
  {
    return { _view, 0 };
  }

  auto end() const noexcept -> iterator
  {
    return { _view, static_cast<uint>(_view.size()) };
  }

  auto substr(UTF8Iterator beg, UTF8Iterator end) const
  {
    return _view.substr(beg._idx, end._idx - beg._idx);
  }

private:
  std::string_view _view;
};

}
