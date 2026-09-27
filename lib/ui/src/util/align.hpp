#pragma once

#include <stdint.h>

namespace tk {

inline constexpr auto align(size_t value, size_t alignment) noexcept
{
  return (value + (alignment - 1)) & ~(alignment - 1);
}

}
