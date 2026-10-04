#pragma once

#include <type_traits>
#include <stdalign.h>
#include <string_view>

#define XXH_INLINE_ALL
#include <xxhash.h>
#include <constexpr-xxh3.h>

namespace tk {

template <typename T>
consteval auto hash(T const* p, size_t n) noexcept -> uint64_t
{
  return constexpr_xxh3::XXH3_64bits_const(p, n);
}

template <typename T>
void hash_update(XXH3_state_t* state, T const& value) noexcept
{
  if constexpr (std::convertible_to<T, std::string_view>)
  {
    auto str = std::string_view{ value };
    XXH3_64bits_update(state, str.data(), str.size());
  }
  else if constexpr (std::is_trivially_copyable_v<T>)
  {
    XXH3_64bits_update(state, &value, sizeof(value));
  }
  else
  {
    static_assert(false, "Type is not hashable");
  }
}

template <typename... Args>
auto hash(Args&&... args) noexcept -> uint64_t
{
  XXH3_state_t state;
  XXH3_64bits_reset(&state);
  (hash_update(&state, std::forward<Args>(args)), ...);
  return XXH3_64bits_digest(&state);
}

}
