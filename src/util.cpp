#include "tk/util.hpp"
#include "tk/ErrorHandling.hpp"

#include <fstream>

namespace tk { namespace util {

auto get_file_data(std::string_view filename) -> std::vector<uint32_t>
{
  std::ifstream file(filename.data(), std::ios::ate | std::ios::binary);
  throw_if(!file.is_open(), "failed to open {}", filename);

  auto file_size = (size_t)file.tellg();
  // A SPIR-V module is defined a stream of 32bit words
  auto buffer    = std::vector<uint32_t>(file_size / sizeof(uint32_t));
  
  file.seekg(0);
  file.read(reinterpret_cast<char*>(buffer.data()), file_size);

  file.close();
  return buffer;
}

auto lerp(glm::vec2 const& a, glm::vec2 const& b, float t) -> glm::vec2
{
  return { std::lerp(a.x, b.x, t), std::lerp(a.y, b.y, t) };
  // use round for small pixel level lerp for smooth
  //return { std::round(std::lerp(a.x, b.x, t)), std::round(std::lerp(a.y, b.y, t)) };
}

auto lerp(std::vector<glm::vec2> const& a, std::vector<glm::vec2> const& b, float t) -> std::vector<glm::vec2>
{    
  assert(!a.empty() && a.size() == b.size());
  auto res = std::vector<glm::vec2>();
  res.reserve(a.size());
  for (auto i = 0; i < a.size(); ++i)
    res.emplace_back(lerp(a[i], b[i], t));
  return res;
}

}}