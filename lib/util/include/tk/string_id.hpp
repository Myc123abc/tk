#pragma once

#include "hash.hpp"

#include <string>

namespace tk {

struct StringID
{
  template <size_t N>
  consteval StringID(char const (&str)[N]) noexcept
    : _id(hash(str, N - 1)) {}

  constexpr StringID(uint64_t id) noexcept
    : _id(id) {}
  
  StringID(std::string const& str) noexcept
    : _id(hash(str)) {}

  StringID(std::string_view str) noexcept
    : _id(hash(str)) {}
  
  constexpr operator uint64_t() const noexcept { return _id; }

private:
  uint64_t _id{};
};

struct StringLiteral
{
  consteval StringLiteral() noexcept = default;

  template <size_t N>
  consteval StringLiteral(char const (&str)[N]) noexcept
    : _id(hash(str, N - 1)), _str(str) {}

  StringLiteral(std::string const& str) noexcept
    : _id(hash(str)), _str(str.data()) {}

  StringLiteral(std::string_view str) noexcept
    : _id(hash(str)), _str(str.data()) {}
  
  constexpr auto view() const noexcept { return _str; }
  constexpr auto id()   const noexcept { return _id;  }

  constexpr auto empty() const noexcept { return !_str; }

private:
  uint64_t    _id{};
  char const* _str{};
};

}
