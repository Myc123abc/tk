module;

#include <windows.h>
#include <string_view>
#include <print>

export module tk.error_handling;

import tk.log;

export namespace tk {

inline void exit(std::string_view msg) noexcept
{
  std::println("{}", msg);
  std::exit(EXIT_FAILURE);
}

template <typename... T>
inline void exit(std::format_string<T...> const fmt, T&&... args) noexcept
{
  std::println(fmt, std::forward<T>(args)...);
  std::exit(EXIT_FAILURE);
}

inline void exit_if(bool b, std::string_view msg) noexcept
{
  if (b) [[unlikely]] exit(msg);
}

template <typename... T>
inline void exit_if(bool b, std::format_string<T...> const fmt, T&&... args) noexcept
{
  if (b) [[unlikely]] exit(fmt, std::forward<T>(args)...);
}

inline void err_if(bool b, std::string_view msg) noexcept
{
  if (b) [[unlikely]]
  {
    error(msg);
    std::exit(EXIT_FAILURE);
  }
}

template <typename... T>
inline void err_if(bool b, std::format_string<T...> const fmt, T&&... args) noexcept
{
  if (b) [[unlikely]]
  {
    error(fmt, std::forward<T>(args)...);
    std::exit(EXIT_FAILURE);
  }
}

inline void err_if(HRESULT hr, std::string_view msg) noexcept
{
  err_if(FAILED(hr), msg);
}

template <typename... T>
inline void err_if(HRESULT hr, std::format_string<T...> const fmt, T&&... args) noexcept
{
  err_if(FAILED(hr), fmt, std::forward<T>(args)...);
}

}
