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
  decltype(auto) visit(Fs&&... fs) noexcept
  {
    return std::visit(
      Visitor<std::decay_t<Fs>...>{
        std::forward<Fs>(fs)...
      },
      *this
    );
  }

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

  template <typename T>
  constexpr auto is() const noexcept
  {
    return std::holds_alternative<T>(*this);
  }

  template <typename T>
  constexpr decltype(auto) get() & 
  {
    return std::get<T>(*this);
  }

  template <typename T>
  constexpr decltype(auto) get() const&
  {
    return std::get<T>(*this);
  }

  template <typename T>
  constexpr decltype(auto) get() &&
  {
    return std::get<T>(std::move(*this));
  }

  template <typename T>
  constexpr decltype(auto) get() const&&
  {
    return std::get<T>(std::move(*this));
  }
};

struct IgnoreVariantCase
{
  template <typename T>
  constexpr void operator()(T&&)  const noexcept {}
};
inline constexpr IgnoreVariantCase VariantDefaultCase;

}
