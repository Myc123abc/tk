#pragma once

#include "util/log.hpp"

#include <windows.h>

namespace tk {

inline void err_if(bool b, std::string_view msg) noexcept
{
  if (b) [[unlikely]]
  {
    error(msg);
    exit(EXIT_FAILURE);
  }
}

template <typename... T>
inline void err_if(bool b, std::format_string<T...> const fmt, T&&... args) noexcept
{
  if (b) [[unlikely]]
  {
    error(fmt, std::forward<T>(args)...);
    exit(EXIT_FAILURE);
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
