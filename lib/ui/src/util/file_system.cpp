#include "file_system.hpp"
#include "iocp.hpp"

#include "tk/error_handling.hpp"

#include <filesystem>
#include <limits>
#include <utility>

namespace fs = std::filesystem;

namespace tk {

void FileSystem::destroy() noexcept
{
  for (auto& [id, file] : _files)
    file.destroy();
  _files.clear();
}

auto FileSystem::exists(std::string_view path) noexcept -> bool
{
  return fs::exists(path);
}

auto FileSystem::create_directories(std::string_view path) noexcept -> bool
{
  auto error = std::error_code{};
  auto const created = fs::create_directories(path, error);
  err_if(static_cast<bool>(error), "failed to create directories {}: {}", path, error.message());
  return created;
}

auto FileSystem::load(StringLiteral path) noexcept -> std::expected<File const*, FileLoadErrorType>
{
  if (!exists(path.view())) return std::unexpected(FileLoadError::unexist{});

  auto [it, inserted] = _files.try_emplace(path.id());
  auto& file = it->second;
  if (!inserted && !file.is_complete())
    return std::unexpected(FileLoadError::loading{});

  auto const last_write_time = fs::last_write_time(path.view());
  if (inserted || file._needs_read || file._last_write_time != last_write_time)
  {
    if (!inserted) file.destroy();
    file.init(path.view(), File::Access::read, File::Share::read, File::Dispostion::open_existing);
    file._last_write_time = last_write_time;
  }

  if (!file.is_complete()) return std::unexpected(FileLoadError::loading{});
  file.close();
  return &file;
}

auto FileSystem::write(StringLiteral path, std::span<uint8 const> data, bool is_append) noexcept -> std::expected<File const*, FileWriteErrorType>
{
  auto& file = _files[path.id()];
  if (!file.is_complete()) return std::unexpected(FileWriteError::writing{});

  auto const parent = fs::path(path.view()).parent_path();
  if (!parent.empty())
    create_directories(parent.string());

  // Save file data, avoiding data be released which is current file's data.
  auto previous_data = std::move(file._data);
  file.destroy();
  file.init(path.view(), File::Access::write, File::Share::read,
    is_append ? File::Dispostion::open_always : File::Dispostion::create_always);

  auto const offset = file._size;
  err_if(data.size() > std::numeric_limits<uint64>::max() - offset,
    "file {} exceeds the supported size", path.view());
  g_iocp.write(file._handle, data, offset);
  file._size       = offset + static_cast<uint64>(data.size());
  file._needs_read = true;
  return &file;
}

auto FileSystem::is_updated(StringLiteral path) noexcept -> bool
{
  if (!exists(path.view())) return true;
  auto const it = _files.find(path.id());
  return it == _files.end() || it->second._needs_read
    || it->second._last_write_time != fs::last_write_time(path.view());
}

void File::init(std::string_view path, Flag<Access> access, Flag<Share> share, Dispostion disposition) noexcept
{
  assert(_handle == INVALID_HANDLE_VALUE);

  // create file
  _handle = CreateFileA(path.data(), access.to<DWORD>(), share.to<DWORD>(), nullptr, static_cast<DWORD>(disposition),
    FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED, nullptr);
  err_if(_handle == INVALID_HANDLE_VALUE, "failed to create file {}", path);

  // get size
  auto li = LARGE_INTEGER{};
  err_if(!GetFileSizeEx(_handle, &li), "failed to get size of file {}", path);
  _size = static_cast<uint64>(li.QuadPart);

  if (access.contains(Access::read) && _size != 0)
  {
    _data = std::make_unique_for_overwrite<uint8[]>(_size);
    g_iocp.read(_handle, { _data.get(), _size });
  }
}

void File::destroy() noexcept
{
  close();
  _data.reset();
  _size            = {};
  _last_write_time = {};
  _needs_read      = false;
}

void File::close() noexcept
{
  if (_handle != INVALID_HANDLE_VALUE)
  {
    while (!is_complete())
      g_iocp.poll(INFINITE);
    err_if(!CloseHandle(_handle), "failed to close file handle");
    _handle = INVALID_HANDLE_VALUE;
  }
}

auto File::is_complete() const noexcept -> bool
{
  return _handle == INVALID_HANDLE_VALUE || g_iocp.is_complete(_handle);
}

}
