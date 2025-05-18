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

}}