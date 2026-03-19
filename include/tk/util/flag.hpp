#pragma once

#include <type_traits>

#define Flag(Type, ...)\
enum class Type : uint32_t { __VA_ARGS__ };\
\
template <typename >\
constexpr auto operator&(Type lhs, Type rhs) noexcept\
{\
  using U = std::underlying_type_t<Type>;\
  return static_cast<Type>(static_cast<U>(lhs) & static_cast<U>(rhs));\
}\
\
template <typename Type>\
constexpr auto operator|(Type lhs, Type rhs) noexcept\
{\
  using U = std::underlying_type_t<Type>;\
  return static_cast<Type>(static_cast<U>(lhs) | static_cast<U>(rhs));\
}\
\
template <typename Type>\
constexpr auto operator^(Type lhs, Type rhs) noexcept\
{\
  using U = std::underlying_type_t<Type>;\
  return static_cast<Type>(static_cast<U>(lhs) ^ static_cast<U>(rhs));\
}\
\
template <typename Type>\
constexpr auto operator~(Type v) noexcept\
{\
  using U = std::underlying_type_t<Type>;\
  return static_cast<Type>(~static_cast<U>(v));\
}\
\
template <typename Type>\
constexpr auto& operator&=(Type& lhs, Type rhs) noexcept\
{\
  return lhs = lhs & rhs;\
}\
\
template <typename Type>\
constexpr auto& operator|=(Type& lhs, Type rhs) noexcept\
{\
  return lhs = lhs | rhs;\
}\
\
template <typename Type>\
constexpr auto has_flag(Type value, Type flag) noexcept\
{\
  using U = std::underlying_type_t<Type>;\
  return (static_cast<U>(value) & static_cast<U>(flag)) != 0;\
}
