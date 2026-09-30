#pragma once

#include "hash.hpp"

namespace tk {

struct StringID
{
  consteval StringID( ) noexcept = default;

  template <size_t N>
  consteval StringID(char const (&str)[N]) noexcept
    : _id(hash_consteval(str, N - 1)) {}

  constexpr operator uint64_t() const noexcept
  {
    return _id;
  }

private:
  uint64_t _id{};
};

}
