#include "tk/util.hpp"
#include "tk/ErrorHandling.hpp"

#include <fstream>
#include <algorithm>

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

auto to_unicode(std::string_view str) -> std::pair<uint32_t, uint32_t>
{
  assert(!str.empty());
  uint8_t ch = str[0];
  if (ch < 0x80)
    return { ch, 1 };
  else if ((ch & 0xE0) == 0xC0)
  {
    assert(str.size() > 1);
    return { (ch & 0x1F) << 6 | (str[1] & 0x3F), 2 };
  }
  else if ((ch & 0xF0) == 0xE0)
  {
    assert(str.size() > 2);
    return { (ch & 0x0F) << 12 | (str[1] & 0x3F) << 6 | (str[2] & 0x3F), 3 };
  }
  else if ((ch & 0xF8) == 0xF0)
  {
    assert(str.size() > 3);
    return { (str[0] & 0x07) << 18 | (str[1] & 0x3F) << 12 | (str[2] & 0x3F) << 6 | (str[3] & 0x3F), 4 };
  }
  assert(true);
  return {};
}

auto to_u32string(std::string_view str) -> std::u32string
{
  std::u32string u32str;
  for (auto i = 0; i < str.size();)
  {
    auto res = to_unicode(str.data() + i);
    u32str.push_back(res.first);
    i += res.second;
  }
  return u32str;
}

auto to_lower(std::string_view str) -> std::string
{
  std::string res(str.size(), '\0');
  std::transform(str.begin(), str.end(), res.begin(), [](auto ch) { return std::tolower(ch); });
  return res;
}

}}