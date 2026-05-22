#pragma once

#include <variant>

namespace tk {

template<class... T>
struct Visitor : T... { using T::operator()...; };

template<class... T>
Visitor(T...) -> Visitor<T...>;

template<class... Ts>
struct Variant : std::variant<Ts...>
{
  using Base = std::variant<Ts...>;
  using Base::Base;

  template<class... Fs>
  decltype(auto) visit(Fs&&... fs) const noexcept
  {
    return std::visit(
      Visitor<std::decay_t<Fs>...>{
        std::forward<Fs>(fs)...
      },
      *this
    );
  }
};

}
