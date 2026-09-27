#pragma once

#include <tuple>

namespace tk {

template <typename... Ts>
struct Tuple : std::tuple<Ts...>
{
  template <typename F>
  constexpr decltype(auto) apply(F&& func) noexcept
  {
    return std::apply(
      std::forward<F>(func),
      static_cast<std::tuple<Ts...>&>(*this));
  }
};

}
