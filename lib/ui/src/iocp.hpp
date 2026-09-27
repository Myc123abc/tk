#pragma once

#include "tk/error_handling.hpp"
#include "util/unicode.hpp"

#include <optional>
#include <vector>

namespace tk {

inline auto get_win32_err_msg() noexcept -> std::string
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

inline auto overlapped = OVERLAPPED{};
class IOCP
{
public:
  void init() noexcept
  {
    _iocp = CreateIoCompletionPort(INVALID_HANDLE_VALUE, nullptr, 0, 0);
    err_if(!_iocp, "Failed to create IOCP handle");
  }

  void destroy() noexcept
  {
    CloseHandle(_iocp);
  }

  struct PollResult
  {
    DWORD        size{};
    ULONG_PTR    completion_key{};
    LPOVERLAPPED overlapped{};
  };

  auto poll() noexcept -> std::optional<PollResult>
  {
    auto res = PollResult{};
    // TODO: change to Ex versions for batch process
    if (!GetQueuedCompletionStatus(_iocp, &res.size, &res.completion_key, &res.overlapped, 0))
    {
      auto err = GetLastError();
      exit_if(res.overlapped, "Failed to get IOCP completion result.\n"
        "Have overlapped can get expanded error result.\n{}", get_win32_err_msg());
      
      // timeout
      if (err == WAIT_TIMEOUT)
        return {};

      // IOCP was closed
      exit_if(err == ERROR_ABANDONED_WAIT_0, "IOCP be closed!");

      // other errors
      exit_if(true, "Failed to get IOCP completion result. {}", get_win32_err_msg());
    }

    return res;
  }

  void add(HANDLE handle) noexcept
  {
    // TODO: abstract overlapped to file content
    err_if(CreateIoCompletionPort(handle, _iocp, reinterpret_cast<ULONG_PTR>(&overlapped), 0) != _iocp,
      "Failed to associate handle with ICOP");
  }

private:
  HANDLE _iocp{};
};

inline void test_iocp() noexcept
{
  // Open the file for overlapped I/O
  auto file = CreateFileA("../../test/main.cpp", GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_FLAG_OVERLAPPED, nullptr);
  err_if(file == INVALID_HANDLE_VALUE, "Failed to open the file");

  auto iocp = IOCP{};
  iocp.init();

  // Associate the file with IOCP
  iocp.add(file);

  // Get size of file
  auto li = LARGE_INTEGER{};
  err_if(!GetFileSizeEx(file, &li), "Failed to get size of file");
  auto size = static_cast<size_t>(li.QuadPart);
  auto buf = std::vector<uint8_t>(size);

  // Read file async
  ReadFile(file, buf.data(), size, nullptr, &overlapped);

  auto fin = false;
  while (!fin)
  {
    while (auto res = iocp.poll())
    {
      err_if(res->size != size, "Failed to read file completely");
      info("{}", reinterpret_cast<char*>(buf.data()));
      fin = true;
    }
  }

  // TODO: wait all IO operations complete.
  CloseHandle(file);
  iocp.destroy();
}

}
