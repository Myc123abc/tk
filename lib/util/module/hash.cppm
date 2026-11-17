module;

#include <type_traits>
#include <stdalign.h>
#include <string_view>
#include <utility>

#define XXH_STATIC_LINKING_ONLY
#include <xxhash.h>
#include <constexpr-xxh3.h>

export module tk.hash;

namespace tk {

template <typename T>
void hash_update(XXH3_state_t* state, T const& value) noexcept
{
  if constexpr (std::is_convertible_v<T const&, std::string_view>)
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

}

export namespace tk {

template <typename T>
consteval auto hash(T const* p, size_t n) noexcept -> uint64_t
{
  return constexpr_xxh3::XXH3_64bits_const(p, n);
}

template <typename T>
auto hash(T const& value) noexcept -> uint64_t
{
  if constexpr (std::is_convertible_v<T const&, std::string_view>)
  {
    auto str = std::string_view{ value };
    return XXH3_64bits(str.data(), str.size());
  }
  else if constexpr (std::is_trivially_copyable_v<T>)
  {
    return XXH3_64bits(&value, sizeof(value));
  }
  else
  {
    static_assert(false, "Type is not hashable");
  }
}

template <typename First, typename Second, typename... Rest>
auto hash(First&& first, Second&& second, Rest&&... rest) noexcept -> uint64_t
{
  XXH3_state_t state;
  XXH3_64bits_reset(&state);
  hash_update(&state, std::forward<First>(first));
  hash_update(&state, std::forward<Second>(second));
  (hash_update(&state, std::forward<Rest>(rest)), ...);
  return XXH3_64bits_digest(&state);
}

}
