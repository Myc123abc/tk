module;

#include <windows.h>
#include <string_view>
#include <format>

module tk:error_handling;

import :log;

namespace tk {

void err_if(bool b, std::string_view msg) noexcept
{
  if (b)
  {
    error(msg);
    exit(EXIT_FAILURE);
  }
}

template <typename... T>
void err_if(bool b, std::format_string<T...> const fmt, T&&... args) noexcept
{
  if (b)
  {
    error(fmt, std::forward<T>(args)...);
    exit(EXIT_FAILURE);
  }
}

void err_if(HRESULT hr, std::string_view msg) noexcept
{
  err_if(FAILED(hr), msg);
}

template <typename... T>
void err_if(HRESULT hr, std::format_string<T...> const fmt, T&&... args) noexcept
{
  err_if(FAILED(hr), fmt, std::forward<T>(args)...);
}

}
