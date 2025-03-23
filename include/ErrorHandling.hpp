#pragma once

#include <stdexcept>
#include <string_view>

namespace tk
{

inline void throw_if(bool b, std::string_view msg)
{
  if (b) throw std::runtime_error(msg.data());
}

}
