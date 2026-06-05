#pragma once

#include <type_traits>

template <typename T>
requires
  std::is_enum_v<T> &&
  (!std::is_convertible_v<T, std::underlying_type_t<T>>)
class Flag
{
public:
  using U = std::underlying_type_t<T>;

  constexpr Flag() noexcept = default;
  constexpr Flag(T flag) noexcept : _flag(flag) {}

  constexpr auto operator&(Flag rhs) const noexcept -> Flag
  {
    return Flag{ static_cast<T>(static_cast<U>(_flag) & static_cast<U>(rhs._flag)) };
  }

  constexpr auto operator|(Flag rhs) const noexcept -> Flag
  {
    return Flag{ static_cast<T>(static_cast<U>(_flag) | static_cast<U>(rhs._flag)) };
  }

  constexpr auto operator^(Flag rhs) const noexcept -> Flag
  {
    return Flag{ static_cast<T>(static_cast<U>(_flag) ^ static_cast<U>(rhs._flag)) };
  }

  constexpr auto operator~() const noexcept -> Flag
  {
    return Flag{ static_cast<T>(~static_cast<U>(_flag)) };
  }

  constexpr auto operator&=(Flag rhs) noexcept -> Flag&
  {
    _flag = (*this & rhs)._flag;
    return *this;
  }

  constexpr auto operator|=(Flag rhs) noexcept -> Flag&
  {
    _flag = (*this | rhs)._flag;
    return *this;
  }

  constexpr auto operator^=(Flag rhs) noexcept -> Flag&
  {
    _flag = (*this ^ rhs)._flag;
    return *this;
  }

  constexpr auto operator==(Flag rhs) const noexcept -> bool
  {
      return _flag == rhs._flag;
  }

  constexpr auto operator!=(Flag rhs) const noexcept -> bool
  {
      return _flag != rhs._flag;
  }

  constexpr auto empty() const noexcept -> bool
  {
    return static_cast<U>(_flag) == 0;
  }

  constexpr auto contains(Flag flag) const noexcept -> bool
  {
    return (static_cast<U>(_flag) & static_cast<U>(flag._flag)) != 0;
  }

  template <typename... Args>
  constexpr auto all(Args... args) const noexcept -> bool
  {
    return (contains(Flag{ args }) && ...);
  }

  template <typename... Args>
  constexpr auto any(Args... args) const noexcept -> bool
  {
    return (contains(Flag{ args }) || ...);
  }

  constexpr auto value() const noexcept -> T
  {
    return _flag;
  }

  constexpr auto& add(Flag flag) noexcept
  {
    return *this |= flag;
  }

  constexpr auto& remove(Flag flag) noexcept
  {
    return *this &= ~flag;
  }
  
  constexpr auto& clear() noexcept
  {
    _flag = {};
    return *this;
  }

private:
  T _flag{};
};

template <typename T>
requires
  std::is_enum_v<T> &&
  (!std::is_convertible_v<T, std::underlying_type_t<T>>)
constexpr auto operator|(T lhs, T rhs) noexcept -> Flag<T>
{
  return Flag{ lhs } | rhs;
}

template <typename T>
requires
  std::is_enum_v<T> &&
  (!std::is_convertible_v<T, std::underlying_type_t<T>>)
constexpr auto operator^(T lhs, T rhs) noexcept -> Flag<T>
{
  return Flag{ lhs } ^ rhs;
}

template <typename T>
requires
  std::is_enum_v<T> &&
  (!std::is_convertible_v<T, std::underlying_type_t<T>>)
constexpr auto operator~(T flag) noexcept -> Flag<T>
{
  return ~Flag{ flag };
}
