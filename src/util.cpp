#include "tk/util.hpp"
#include "tk/ErrorHandling.hpp"

namespace tk { namespace util {

auto get_file_data(std::string_view filename) -> std::vector<uint32_t>
{
  FILE* file;
  throw_if(fopen_s(&file, filename.data(), "rb"), "failed to open {}", filename);

  fseek(file, 0, SEEK_END);
  auto file_size = ftell(file);
  fseek(file, 0, SEEK_SET);

  // A SPIR-V module is defined a stream of 32bit words
  auto buffer = std::vector<uint32_t>(file_size / sizeof(uint32_t));
  throw_if(fread(buffer.data(), sizeof(uint32_t), buffer.size(), file) != buffer.size(),
           "failed to read full file of {}", filename);

  fclose(file);
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