#pragma once

#include "../singleton.hpp"
#include "tk/base.hpp"

#include <Windows.h>

#include <condition_variable>
#include <coroutine>
#include <mutex>
#include <span>
#include <thread>
#include <unordered_map>
#include <vector>

namespace tk {

Singleton(IOCP, g_iocp,
public:
  void init() noexcept;
  void destroy() noexcept;

  void associate(HANDLE handle) noexcept;
  void read(HANDLE handle, std::span<uint8> data) noexcept;
  void write(HANDLE handle, std::span<uint8 const> data, uint64 offset = 0) noexcept;

  struct Awaiter
  {
    IOCP&  iocp;
    HANDLE handle;

    auto await_ready() const noexcept -> bool;
    auto await_suspend(std::coroutine_handle<> continuation) const noexcept -> bool;
    void await_resume() const noexcept {}
  };

  auto completion(HANDLE handle) noexcept { return Awaiter{ *this, handle }; }

  auto is_complete(HANDLE handle) noexcept -> bool;
  void wait(HANDLE handle) noexcept;

private:
  struct Record;

  void main() noexcept;
  void process(OVERLAPPED_ENTRY const& entry) noexcept;
  void submit(Record& record) noexcept;

private:
  enum class Operation : uint8
  {
    read,
    write,
  };

  struct Record
  {
    OVERLAPPED              overlapped{};
    HANDLE                  file{};
    std::span<uint8>        read_buf;
    std::vector<uint8>      write_buf;
    uint64                  file_offset{};
    uint64                  offset{};
    uint64                  size{};
    DWORD                   pending_size{};
    Operation               operation{};
    std::coroutine_handle<> continuation{};
  };

  static constexpr auto Chunk_Size = DWORD{ 64 * 1024 * 1024 };

  HANDLE                             _iocp{ INVALID_HANDLE_VALUE };
  std::unordered_map<HANDLE, Record> _records;
  std::thread                        _thread;
  std::mutex                         _mutex;
  std::condition_variable            _completed;
  bool                               _stopping{ false };
)

}
