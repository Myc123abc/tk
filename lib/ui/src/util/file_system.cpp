#include "file_system.hpp"
#include "async/iocp.hpp"

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

auto FileSystem::open(StringLiteral path) noexcept -> Task<File*>
{
  auto [it, inserted] = _files.try_emplace(path.id());
  auto& file = it->second;

  if (inserted || file._handle == INVALID_HANDLE_VALUE)
  {
    if (!inserted) file.destroy();

    // create directories if unexists
    auto const parent = fs::path(path.view()).parent_path();
    if (!parent.empty()) create_directories(parent.string());

    // create file
    file.init(path.view(), File::Access::read | File::Access::write, File::Share::read, File::Dispostion::open_always);
    file._last_write_time = fs::last_write_time(path.view());
  }

  co_await g_iocp.completion(file._handle);
  co_return &file;
}

auto FileSystem::load(StringLiteral path) noexcept -> std::expected<File const*, FileLoadErrorType>
{
  auto it = _files.find(path.id());
  if (it != _files.end() && !it->second.is_complete())
    return std::unexpected(FileLoadError::loading{});

  if (!exists(path.view())) return std::unexpected(FileLoadError::unexist{});

  auto const inserted = it == _files.end();
  if (inserted) it = _files.try_emplace(path.id()).first;
  auto& file = it->second;

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

auto File::write(std::span<uint8 const> data, bool is_append) noexcept -> bool
{
  assert(_handle != INVALID_HANDLE_VALUE);
  if (!is_complete()) return false;

  auto const offset = is_append ? _size : uint64{};
  err_if(data.size() > std::numeric_limits<uint64>::max() - offset,
    "file exceeds the supported size");
  auto const size = offset + static_cast<uint64>(data.size());

  if (!is_append)
  {
    auto file_size = FILE_END_OF_FILE_INFO{};
    file_size.EndOfFile.QuadPart = static_cast<LONGLONG>(size);
    err_if(!SetFileInformationByHandle(_handle, FileEndOfFileInfo, &file_size, sizeof(file_size)),
      "failed to resize file");
  }

  g_iocp.write(_handle, data, offset);
  _size       = size;
  _needs_read = true;
  return true;
}

void FileSystem::wait(StringLiteral path) noexcept
{
  auto const it = _files.find(path.id());
  assert(it != _files.end());
  auto const handle = it->second._handle;
  if (handle != INVALID_HANDLE_VALUE)
    g_iocp.wait(handle);
}

auto FileSystem::is_updated(StringLiteral path) noexcept -> bool
{
  auto const it = _files.find(path.id());
  err_if(it == _files.end(), "failed to check update on file {}, is not be loaded", path.view());
  return it->second._needs_read || it->second._last_write_time != fs::last_write_time(path.view());
}

void File::init(std::string_view path, Flag<Access> access, Flag<Share> share, Dispostion disposition) noexcept
{
  assert(_handle == INVALID_HANDLE_VALUE);

  // create file
  _handle = CreateFileA(path.data(), access.to<DWORD>(), share.to<DWORD>(), nullptr, static_cast<DWORD>(disposition),
    FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED, nullptr);
  err_if(_handle == INVALID_HANDLE_VALUE, "failed to create file {}", path);
  g_iocp.associate(_handle);

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
    g_iocp.wait(_handle);
    err_if(!CloseHandle(_handle), "failed to close file handle");
    _handle = INVALID_HANDLE_VALUE;
  }
}

auto File::is_complete() const noexcept -> bool
{
  return _handle == INVALID_HANDLE_VALUE || g_iocp.is_complete(_handle);
}

void File::wait() const noexcept
{
  if (!is_complete()) g_iocp.wait(_handle);
}

}
