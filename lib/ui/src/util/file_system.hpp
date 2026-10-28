#pragma once

#include "singleton.hpp"
#include "tk/string_id.hpp"
#include "tk/flag.hpp"
#include "tk/variant.hpp"
#include "tk/base.hpp"
#include "async/task.hpp"

#include <unordered_map>
#include <expected>
#include <filesystem>
#include <memory>
#include <span>

#include <windows.h>

namespace tk {

namespace FileLoadError {
struct unexist {};
struct loading {};
using Type = Variant<unexist, loading>;
}
using FileLoadErrorType = FileLoadError::Type;

namespace FileWriteError {
struct writing {};
using Type = Variant<writing>;
}
using FileWriteErrorType = FileWriteError::Type;

struct File
{
  friend class FileSystem;

  File() noexcept              = default;
  File(File const&)            = delete;
  File& operator=(File const&) = delete;

  enum class Access : DWORD
  {
    read  = GENERIC_READ,
    write = GENERIC_WRITE,
  };

  enum class Share : DWORD
  {
    none  = 0,
    read  = FILE_SHARE_READ,
    write = FILE_SHARE_WRITE,
    del   = FILE_SHARE_DELETE,
  };

  enum class Dispostion : DWORD
  {
    create_always     = CREATE_ALWAYS,
    create_new        = CREATE_NEW,
    open_always       = OPEN_ALWAYS,
    open_existing     = OPEN_EXISTING,
    truncate_existing = TRUNCATE_EXISTING,
  };

  void init(std::string_view path, Flag<Access> access, Flag<Share> share, Dispostion disposition) noexcept;
  void destroy() noexcept;

  auto write(std::span<uint8 const> data, bool is_append) noexcept -> bool;

  template <typename T>
  auto data() const noexcept { return reinterpret_cast<T*>(_data.get()); }
  auto data() const noexcept { return _data.get(); }
  auto size() const noexcept { return _size; }

  auto is_complete() const noexcept -> bool;
  void wait() const noexcept;

private:
  void close() noexcept;

private:
  HANDLE                          _handle{ INVALID_HANDLE_VALUE };
  uint64                          _size{};
  std::unique_ptr<uint8[]>        _data;
  std::filesystem::file_time_type _last_write_time{};
  bool                            _needs_read{};
};

Singleton(FileSystem, g_file_sys,
public:
  void destroy() noexcept;

  static auto exists(std::string_view path) noexcept -> bool;
  static auto create_directories(std::string_view path) noexcept -> bool;

  auto open(StringLiteral path) noexcept -> Task<File*>;
  auto load(StringLiteral path) noexcept -> std::expected<File const*, FileLoadErrorType>;
  void wait(StringLiteral path) noexcept;

  auto is_updated(StringLiteral path) noexcept -> bool;

private:
  std::unordered_map<StringID, File> _files;
)

}
