#pragma once

#include <vector>
#include <string>

#include <glm/glm.hpp>

namespace tk { namespace util {
    
inline uint32_t align_size(uint32_t size, uint32_t alignment) noexcept
{
  return (size + alignment - 1) & ~(alignment - 1);
}

auto get_file_data(std::string_view filename) -> std::vector<uint32_t>;

auto lerp(glm::vec2 const& a, glm::vec2 const& b, float t) -> glm::vec2;
auto lerp(std::vector<glm::vec2> const& a, std::vector<glm::vec2> const& b, float t) -> std::vector<glm::vec2>;

auto to_lower(std::string_view str) -> std::string;

}}