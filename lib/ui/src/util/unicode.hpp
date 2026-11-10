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

struct UTF8Codepoint
{
  uint codepoint{};
  uint size{};
  bool valid{};
};

inline auto next_codepoint(std::string_view str, uint idx) noexcept -> UTF8Codepoint
{
  if (idx >= str.size()) return {};

  auto const first_byte = static_cast<uint8>(str[idx]);
  auto const size       = utf8_sequence_size(first_byte);
  if (!size) return { 0xfffd, 1, false };

  auto res = static_cast<uint>(first_byte & UTF8_First_Byte_Masks[size]);

  // Truncated sequence
  if (idx + size > str.size())
    return { 0xfffd, static_cast<uint>(str.size() - idx), false };

  // Validate continuation bytes
  for (auto i = 1; i < size; ++i)
  {
    auto const byte = static_cast<uint8>(str[idx + i]);
    if ((byte & 0xc0) != 0x80)
      return { 0xfffd, 1, false };

    res = (res << 6) | (byte & 0x3f);
  }

  // Reject overlong encoding
  if (res < UTF8_Min_Codepoints[size])
    return { 0xfffd, 1, false };

  // Reject utf16 surrogate range
  if (res >= 0xd800 && res <= 0xdfff)
    return { 0xfffd, 1, false };

  // Reject values outside unicode
  if (res > 0x10ffff)
    return { 0xfffd, 1, false };

  return { res, size, true };
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
    auto const decoded = next_codepoint(str, idx);
    err_if(!decoded.valid, "Invalid u8string: {}", str);
    idx += decoded.size;
    ++cnt;
  }
  return cnt;
}

}
