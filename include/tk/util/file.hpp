#pragma once

#include <windows.h>

#include <vector>
#include <string_view>

namespace tk {

class File
{
public:
  explicit File(std::string_view path) noexcept
  {
    _file = CreateFileA(path.data(), GENERIC_READ, FILE_SHARE_READ,
      nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (_file == INVALID_HANDLE_VALUE) return;

    auto li = LARGE_INTEGER{};
    if (!GetFileSizeEx(_file, &li)) return;
    _size = static_cast<size_t>(li.QuadPart);

    if (_size > 1'000'000)
    {
      _mapping = CreateFileMapping(_file, nullptr, PAGE_READONLY, 0, 0, nullptr);
      if (!_mapping) return;
      _data = static_cast<std::byte*>(MapViewOfFile(_mapping, FILE_MAP_READ, 0, 0, 0));
    }
    else
    {
      _buffer.resize(_size);
      auto bytesRead = DWORD{};
      if (!ReadFile(_file, _buffer.data(), static_cast<DWORD>(_size), &bytesRead, nullptr)) return;
      _data = _buffer.data();
    }
  }

  ~File() noexcept
  {
    if (_mapping)
    {
      UnmapViewOfFile(_data);
      CloseHandle(_mapping);
    }
    if (_file != INVALID_HANDLE_VALUE)
      CloseHandle(_file);
  }

  File(const File&)            = delete;
  File& operator=(const File&) = delete;

  operator bool() const noexcept { return _data; }

  template <typename T>
  auto data() const noexcept { return reinterpret_cast<T*>(_data); }
  auto data() const noexcept { return _data; }
  auto size() const noexcept { return _size; }

private:
  HANDLE                 _file{ INVALID_HANDLE_VALUE };
  HANDLE                 _mapping{};
  std::byte*             _data{};
  std::vector<std::byte> _buffer;
  size_t                 _size{};
};

}
