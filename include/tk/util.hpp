#pragma once

#include <vector>
#include <string_view>

namespace tk { namespace util {
    
inline uint32_t align_size(uint32_t size, uint32_t alignment)
{
  return (size + alignment - 1) & ~(alignment - 1);
}

auto get_file_data(std::string_view filename) -> std::vector<uint32_t>;

}}