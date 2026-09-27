#pragma once

#include "singleton.hpp"
#include "object_pool.hpp"
#include "tk/flag.hpp"

#include <windows.h>

#include <filesystem>
#include <unordered_map>

namespace tk {

Singleton(FileManager, g_file_mgr,
private:
  class File
  {
  public:
    File() noexcept              = default;
    File(const File&)            = delete;
    File& operator=(const File&) = delete;

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

    enum class Disposition : DWORD
    {
      create_always     = CREATE_ALWAYS,
      create_new        = CREATE_NEW,
      open_always       = OPEN_ALWAYS,
      open_existing     = OPEN_EXISTING,
      truncate_existing = TRUNCATE_EXISTING,
    };

    void init(std::string_view path, Flag<Access> access_flag, Flag<Share> share_flag, Disposition disposition) noexcept;
    void destroy() noexcept;

    enum class OpenMode
    {
      share_read_existing,
    };
    template <OpenMode Mode>
    void open(std::string_view path)
    {
      if constexpr (Mode == OpenMode::share_read_existing)
        init(path, Access::read, Share::read, Disposition::open_existing);
      else
        static_assert(false, "Unsupported OpenMode");
    }

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

  using FilePoolType = ObjectPool<File>;
public:
  using FileHandle   = FilePoolType::Handle;

public:
  void destroy() noexcept;

  auto exists(std::string_view path) noexcept -> bool;

  auto load(std::string_view path) noexcept -> FileHandle;

  auto& operator[](FileHandle handle) noexcept { return _pool[handle]; }

  auto is_updated(std::string_view path) noexcept -> bool;

private:
  enum class FileStatus
  {
    unexist = 1 << 0,
    exist   = 1 << 1,
    loaded  = 1 << 2,
  };

  struct FileInfo
  {
    FileHandle                      handle;
    Flag<FileStatus>                status{};
    std::filesystem::file_time_type last_write_time;
  };
  std::unordered_map<std::string, FileInfo> _infos;
  FilePoolType                              _pool;
)

using FileHandle = FileManager::FileHandle;

}
