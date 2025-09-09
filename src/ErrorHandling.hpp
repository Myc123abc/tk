#pragma once

#include <stdexcept>
#include <format>

namespace tk
{

  template <typename T>
  inline void throw_if(bool b, T const& msg)
  {
    if (b) throw std::runtime_error(msg);
  }
  
  template <typename... Args>
  inline void throw_if(bool b, std::format_string<Args...> fmt, Args&&... args)
  {
    if (b) throw std::runtime_error(std::format(fmt, std::forward<Args>(args)...));
  }

}
