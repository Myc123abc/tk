#pragma once

#include <type_traits>
#include <stdalign.h>

#define XXH_INLINE_ALL
#include <xxhash.h>
#include <constexpr-xxh3.h>

namespace tk {

template <typename T>
constexpr void xxh3_update(XXH3_state_t* state, T&& arg) noexcept
{
  using Type = std::remove_cvref_t<T>;
  XXH3_64bits_update(state, &arg, sizeof(Type));
}

template <typename... Args>
constexpr auto hash(Args&&... args) noexcept -> uint64_t
{
  alignas(64) thread_local XXH3_state_t state;
  XXH3_64bits_reset(&state);
  (xxh3_update(&state, std::forward<Args>(args)), ...);
  return XXH3_64bits_digest(&state);
}

template <typename T>
consteval auto hash_consteval(T const* p, size_t n) noexcept -> uint64_t
{
  return constexpr_xxh3::XXH3_64bits_const(p, n);
}

}
