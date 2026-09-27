#include "file_manager.hpp"
#include "tk/error_handling.hpp"

#include <ranges>

namespace tk {

void FileManager::File::init(std::string_view path, Flag<Access> access_flag, Flag<Share> share_flag, Disposition disposition) noexcept
{
  assert(_file == INVALID_HANDLE_VALUE);

  // create file
  _file = CreateFileA(path.data(), access_flag.to<DWORD>(), share_flag.to<DWORD>(),
    nullptr, static_cast<DWORD>(disposition), FILE_ATTRIBUTE_NORMAL, nullptr);
  err_if(_file == INVALID_HANDLE_VALUE, "failed to initialize file {}", path);

  if (disposition != Disposition::open_always && disposition != Disposition::open_existing)
    return;

  // get size
  auto li = LARGE_INTEGER{};
  err_if(!GetFileSizeEx(_file, &li), "failed to get size of file {}", path);
  _size = static_cast<size_t>(li.QuadPart);

  // big file, use file mipping
  if (_size > 1'000'000)
  {
    _mapping = CreateFileMapping(_file, nullptr, PAGE_READONLY, 0, 0, nullptr);
    err_if(!_mapping, "failed to create file mapping of file {}", path);
    _data = static_cast<std::byte*>(MapViewOfFile(_mapping, FILE_MAP_READ, 0, 0, 0));
    err_if(!_data, "failed to map file {}", path);
  }
  // small file, directly read
  else
  {
    _buffer.resize(_size);
    auto bytesRead = DWORD{};
    err_if(!ReadFile(_file, _buffer.data(), static_cast<DWORD>(_size), &bytesRead, nullptr),
      "failed to read file {}", path);

    _data = _buffer.data();
  }

  CloseHandle(_file);
  _file = INVALID_HANDLE_VALUE;
}

void FileManager::File::destroy() noexcept
{
  if (_data && _mapping) UnmapViewOfFile(_data);
  if (_mapping) CloseHandle(_mapping);
  if (_file != INVALID_HANDLE_VALUE) CloseHandle(_file);
  _file    = INVALID_HANDLE_VALUE;
  _mapping = {};
  _data    = {};
  _size    = {};
  _buffer.clear();
}

void FileManager::destroy() noexcept
{
  for (auto& info : _infos | std::views::values) 
  {
    if (info.handle.valid())
    {
      _pool[info.handle].destroy();
      _pool.free(info.handle);
    }
  }
  _infos.clear();
}

auto FileManager::exists(std::string_view path) noexcept -> bool
{
  auto b = std::filesystem::exists(path);
  b ? _infos[path.data()].status.add(FileStatus::exist)
    : _infos[path.data()].status.remove(FileStatus::exist);
  return b;
}

auto FileManager::load(std::string_view path) noexcept -> FileHandle
{
  auto& info = _infos[path.data()];

  assert(info.status.contains(FileStatus::exist));

  // if file is cached
  if (info.status.contains(FileStatus::loaded))
  {
    // check the write time
    auto last_write_time = std::filesystem::last_write_time(path);
    if (last_write_time == info.last_write_time) return info.handle;

    // the file content is changed, reload new one
    auto& file = _pool[info.handle];
    file.destroy();
    file.open<File::OpenMode::share_read_existing>(path.data());
    info.last_write_time = last_write_time;
    return info.handle;
  }

  // cache file
  info.handle = _pool.alloc();
  _pool[info.handle].open<File::OpenMode::share_read_existing>(path.data());
  info.status.add(FileStatus::loaded);
  info.last_write_time = std::filesystem::last_write_time(path);
  return info.handle;
}

auto FileManager::is_updated(std::string_view path) noexcept -> bool
{
  if (!exists(path)) return true;
  assert(_infos.contains(path.data()));
  return _infos[path.data()].last_write_time != std::filesystem::last_write_time(path);
}

}
