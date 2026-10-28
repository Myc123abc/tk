#include "iocp.hpp"
#include "tk/error_handling.hpp"
#include "../unicode.hpp"

#include <array>

namespace tk {

namespace {

auto get_win32_err_msg() noexcept -> std::string
{
  auto buf = LPVOID{};
  auto err = GetLastError();
  if (FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
    NULL, err, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), reinterpret_cast<LPWSTR>(&buf), 0, NULL))
  {
    auto msg = to_string(reinterpret_cast<wchar_t*>(buf));
    LocalFree(buf);
    return msg;
  }
  return "Failed to format message";
}

}

void IOCP::init() noexcept
{
  _iocp     = CreateIoCompletionPort(INVALID_HANDLE_VALUE, nullptr, 0, 0);
  err_if(!_iocp, "Failed to create IOCP handle");
  _stopping = false;
  _thread   = std::thread(&IOCP::main, this);
}

void IOCP::destroy() noexcept
{
  {
    auto lock = std::unique_lock{ _mutex };
    _completed.wait(lock, [&] { return _records.empty(); });
    _stopping = true;
  }

  err_if(!PostQueuedCompletionStatus(_iocp, 0, 0, nullptr),
    "Failed to stop IOCP completion thread: {}", get_win32_err_msg());
  _thread.join();
  err_if(!CloseHandle(_iocp), "Failed to close IOCP handle: {}", get_win32_err_msg());
  _iocp = INVALID_HANDLE_VALUE;
}

void IOCP::main() noexcept
{
  auto entries = std::array<OVERLAPPED_ENTRY, 64>{};
  while (true)
  {
    auto cnt = ULONG{};
    if (!GetQueuedCompletionStatusEx(_iocp, entries.data(), static_cast<ULONG>(entries.size()), &cnt, INFINITE, FALSE))
    {
      auto const err = GetLastError();
      exit_if(err == ERROR_ABANDONED_WAIT_0, "IOCP was closed unexpectedly");
      exit("Failed to get IOCP completion result. {}", get_win32_err_msg());
    }

    for (auto i = 0; i < cnt; ++i)
    {
      if (!entries[i].lpOverlapped) return;
      process(entries[i]);
    }
  }
}

void IOCP::process(OVERLAPPED_ENTRY const& entry) noexcept
{
  auto lock = std::unique_lock{ _mutex };
  assert(entry.lpOverlapped);
  auto record = CONTAINING_RECORD(entry.lpOverlapped, Record, overlapped);
  assert(_records.contains(record->file));

  auto transferred = DWORD{};
  err_if(!GetOverlappedResult(record->file, &record->overlapped, &transferred, FALSE),
    "Failed to get overlapped result : {}", get_win32_err_msg());
  err_if(transferred != entry.dwNumberOfBytesTransferred,
    "IOCP transferred-byte count does not match the overlapped result");
  err_if(transferred > record->pending_size,
    "IOCP transferred more bytes than requested");
  err_if(transferred == 0, "IOCP operation completed without making progress");

  record->offset += transferred;
  if (record->offset == record->size)
  {
    auto const continuation = record->continuation;
    _records.erase(record->file);
    lock.unlock();
    _completed.notify_all();
    if (continuation) continuation.resume();
  }
  else
    submit(*record);
}

void IOCP::submit(Record& record) noexcept
{
  assert(record.offset < record.size);

  auto const remaining = record.size - record.offset;
  record.pending_size  = static_cast<DWORD>(std::min<uint64>(remaining, Chunk_Size));
  record.overlapped    = {};
  auto const file_offset = record.file_offset + record.offset;
  record.overlapped.Offset     = static_cast<DWORD>(file_offset);
  record.overlapped.OffsetHigh = static_cast<DWORD>(file_offset >> 32);

  auto const offset = static_cast<size_t>(record.offset);
  auto* data = record.operation == Operation::read
    ? static_cast<void*>(record.read_buf.data()  + offset)
    : static_cast<void*>(record.write_buf.data() + offset);
  auto const succeeded = record.operation == Operation::read
    ? ReadFile(record.file, data, record.pending_size, nullptr, &record.overlapped)
    : WriteFile(record.file, data, record.pending_size, nullptr, &record.overlapped);
  err_if(!succeeded && GetLastError() != ERROR_IO_PENDING,
    "Failed to start asynchronous file operation : {}", get_win32_err_msg());
}

void IOCP::associate(HANDLE handle) noexcept
{
  assert(handle != nullptr && handle != INVALID_HANDLE_VALUE);
  auto lock = std::lock_guard{ _mutex };
  err_if(_stopping, "Cannot associate a file handle with IOCP while stopping");
  err_if(CreateIoCompletionPort(handle, _iocp, 0, 0) != _iocp,
    "Failed to associate handle with IOCP : {}", get_win32_err_msg());
}

void IOCP::read(HANDLE handle, std::span<uint8> data) noexcept
{
  assert(handle != nullptr && handle != INVALID_HANDLE_VALUE);
  auto lock = std::lock_guard{ _mutex };
  err_if(_stopping, "Cannot submit an IOCP read while stopping");
  assert(!_records.contains(handle));
  if (data.empty())
    return;

  auto& record     = _records[handle];
  record.file      = handle;
  record.size      = data.size();
  record.operation = Operation::read;
  record.read_buf  = data;

  submit(record);
}

void IOCP::write(HANDLE handle, std::span<uint8 const> data, uint64 offset) noexcept
{
  assert(handle != nullptr && handle != INVALID_HANDLE_VALUE);
  auto lock = std::lock_guard{ _mutex };
  err_if(_stopping, "Cannot submit an IOCP write while stopping");
  assert(!_records.contains(handle));
  if (data.empty())
    return;

  auto& record       = _records[handle];
  record.file        = handle;
  record.size        = data.size();
  record.file_offset = offset;
  record.operation   = Operation::write;
  record.write_buf.assign(data.begin(), data.end());

  submit(record);
}

auto IOCP::Awaiter::await_ready() const noexcept -> bool
{
  assert(handle != nullptr && handle != INVALID_HANDLE_VALUE);
  auto lock = std::lock_guard{ iocp._mutex };
  return !iocp._records.contains(handle);
}

auto IOCP::Awaiter::await_suspend(std::coroutine_handle<> continuation) const noexcept -> bool
{
  assert(handle != nullptr && handle != INVALID_HANDLE_VALUE);
  auto lock = std::lock_guard{ iocp._mutex };
  auto const it = iocp._records.find(handle);
  if (it == iocp._records.end()) return false;

  assert(!it->second.continuation);
  it->second.continuation = continuation;
  return true;
}

auto IOCP::is_complete(HANDLE handle) noexcept -> bool
{
  assert(handle != nullptr && handle != INVALID_HANDLE_VALUE);
  auto lock = std::lock_guard{ _mutex };
  return !_records.contains(handle);
}

void IOCP::wait(HANDLE handle) noexcept
{
  assert(handle != nullptr && handle != INVALID_HANDLE_VALUE);
  auto lock = std::unique_lock{ _mutex };
  _completed.wait(lock, [&] { return !_records.contains(handle); });
}

}
