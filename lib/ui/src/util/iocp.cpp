#include "iocp.hpp"
#include "tk/error_handling.hpp"
#include "unicode.hpp"

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
  _iocp = CreateIoCompletionPort(INVALID_HANDLE_VALUE, nullptr, 0, 0);
  err_if(!_iocp, "Failed to create IOCP handle");
}

void IOCP::destroy() noexcept
{
  poll_all_events();
  CloseHandle(_iocp);
}

void IOCP::poll_all_events() noexcept
{
  while (!_records.empty())
    poll(INFINITE);
}

void IOCP::poll(DWORD milli) noexcept
{
  auto cnt     = ULONG{};
  auto entries = std::array<OVERLAPPED_ENTRY, 64>{};
  if (!GetQueuedCompletionStatusEx(_iocp, entries.data(), static_cast<ULONG>(entries.size()), &cnt, milli, FALSE))
  {
    if (auto err = GetLastError(); err != WAIT_TIMEOUT)
    {
      exit_if(err == ERROR_ABANDONED_WAIT_0, "IOCP be closed!");
      exit("Failed to get IOCP completion result. {}", get_win32_err_msg());
    }
  }
  for (auto i = 0; i < cnt; ++i)
    process(entries[i]);
}

void IOCP::process(OVERLAPPED_ENTRY const& entry) noexcept
{
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
    _records.erase(record->file);
  else
    submit(*record);
}

void IOCP::submit(Record& record) noexcept
{
  assert(record.offset < record.size);

  auto const remaining = record.size - record.offset;
  record.pending_size  = static_cast<DWORD>(std::min<uint64>(remaining, Chunk_Size));
  record.overlapped    = {};
  record.overlapped.Offset     = static_cast<DWORD>(record.offset);
  record.overlapped.OffsetHigh = static_cast<DWORD>(record.offset >> 32);

  auto* data = record.operation == Operation::read
    ? record.buf.data()
    : record.buf.data() + static_cast<size_t>(record.offset);
  auto const succeeded = record.operation == Operation::read
    ? ReadFile(record.file, data, record.pending_size, nullptr, &record.overlapped)
    : WriteFile(record.file, data, record.pending_size, nullptr, &record.overlapped);
  err_if(!succeeded && GetLastError() != ERROR_IO_PENDING,
    "Failed to start asynchronous file operation : {}", get_win32_err_msg());
}

void IOCP::read(HANDLE handle) noexcept
{
  assert(handle != nullptr && handle != INVALID_HANDLE_VALUE);
  assert(!_records.contains(handle));

  auto size = LARGE_INTEGER{};
  err_if(!GetFileSizeEx(handle, &size), "Failed to get file size : {}", get_win32_err_msg());
  err_if(size.QuadPart < 0, "Cannot read a file with a negative size");
  if (size.QuadPart == 0)
    return;

  auto& record     = _records[handle];
  record.file      = handle;
  record.size      = static_cast<uint64>(size.QuadPart);
  record.operation = Operation::read;
  record.buf.resize(static_cast<size_t>(std::min<uint64>(record.size, Chunk_Size)));

  err_if(CreateIoCompletionPort(handle, _iocp, 0, 0) != _iocp,
    "Failed to associate handle with IOCP : {}", get_win32_err_msg());

  submit(record);
}

void IOCP::write(HANDLE handle, std::span<uint8 const> data) noexcept
{
  assert(handle != nullptr && handle != INVALID_HANDLE_VALUE);
  assert(!_records.contains(handle));
  if (data.empty())
    return;

  auto& record     = _records[handle];
  record.file      = handle;
  record.size      = data.size();
  record.operation = Operation::write;
  record.buf.assign(data.begin(), data.end());

  err_if(CreateIoCompletionPort(handle, _iocp, 0, 0) != _iocp,
    "Failed to associate handle with IOCP : {}", get_win32_err_msg());

  submit(record);
}

}
