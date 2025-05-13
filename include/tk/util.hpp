#pragma once

#include <cstdint>

namespace tk { namespace util {
    
inline uint32_t align_size(uint32_t size, uint32_t alignment)
{
  return (size + alignment - 1) & ~(alignment - 1);
}

}}