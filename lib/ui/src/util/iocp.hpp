#pragma once

#include "singleton.hpp"
#include "tk/base.hpp"

#include <Windows.h>

#include <span>
#include <unordered_map>

namespace tk {

Singleton(IOCP, g_iocp,
public:
  void init() noexcept;
  void destroy() noexcept;

  void poll(DWORD milli = 0) noexcept;

  void read(HANDLE handle) noexcept;
  void write(HANDLE handle, std::span<uint8 const> data) noexcept;

private:
  struct Record;

  void poll_all_events() noexcept;
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
    OVERLAPPED         overlapped{};
    HANDLE             file{};
    std::vector<uint8> buf;
    uint64             offset{};
    uint64             size{};
    DWORD              pending_size{};
    Operation          operation{};
  };

  static constexpr auto Chunk_Size = DWORD{ 64 * 1024 * 1024 };

  HANDLE                             _iocp{ INVALID_HANDLE_VALUE };
  std::unordered_map<HANDLE, Record> _records;
)

}
