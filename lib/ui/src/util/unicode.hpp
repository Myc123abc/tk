#pragma once

#include "tk/error_handling.hpp"
#include "tk/base.hpp"

// TODO: abstract UnicodeString

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

inline constexpr uint  UTF8_Min_Codepoints[]     { 0, 0,    0x80, 0x800, 0x10000 };
inline constexpr uint8 UTF8_First_Byte_Masks[]   { 0, 0x7f, 0x1f, 0x0f,  0x07    };
inline constexpr uint8 UTF8_First_Byte_Prefixes[]{ 0, 0,    0xc0, 0xe0,  0xf0    };

inline auto utf8_sequence_size(uint8 first_byte) noexcept -> uint
{
  if (first_byte <= 0x7f)          return 1;
  if ((first_byte & 0xe0) == 0xc0) return 2;
  if ((first_byte & 0xf0) == 0xe0) return 3;
  if ((first_byte & 0xf8) == 0xf0) return 4;
  return 0;
}

inline auto utf8_codepoint_size(uint codepoint) noexcept -> uint
{
  if (codepoint <= 0x7f)   return 1;
  if (codepoint <= 0x7ff)  return 2;
  if (codepoint <= 0xffff) return 3;
  return 4;
}

inline auto next_codepoint(std::string_view str, uint idx) -> std::pair<uint, uint>
{
  if (idx >= str.size()) return {};

  auto const first_byte = static_cast<uint8>(str[idx]);
  auto const size       = utf8_sequence_size(first_byte);
  if (!size) return { 0xfffd, 1 };

  auto res = static_cast<uint>(first_byte & UTF8_First_Byte_Masks[size]);

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
  if (res < UTF8_Min_Codepoints[size])
    return { 0xfffd, 1 };

  // Reject utf16 surrogate range
  if (res >= 0xd800 && res <= 0xdfff)
    return { 0xfffd, 1 };

  // Reject values outside unicode
  if (res > 0x10ffff)
    return { 0xfffd, 1 };

  return { res, size };
}

inline void append_codepoint(std::string& str, uint codepoint) noexcept
{
  auto const size = utf8_codepoint_size(codepoint);
  char bytes[4]{};
  for (auto i = size - 1; i; --i)
  {
    bytes[i] = static_cast<char>(0x80 | codepoint & 0x3f);
    codepoint >>= 6;
  }
  bytes[0] = static_cast<char>(UTF8_First_Byte_Prefixes[size] | codepoint);
  str.append(bytes, size);
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
