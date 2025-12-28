#pragma once

#include "error_handling.hpp"

#include <windows.h>

#include <latch>

namespace tk {

class Semaphore
{
public:
  Semaphore()                            = default;
  ~Semaphore()                           = default;
  Semaphore(Semaphore const&)            = delete;
  Semaphore(Semaphore&&)                 = delete;
  Semaphore& operator=(Semaphore const&) = delete;
  Semaphore& operator=(Semaphore&&)      = delete;

  void init(uint32_t init_count = 0, uint32_t capacity = LONG_MAX) noexcept
  {
    _sem = CreateSemaphoreA(nullptr, init_count, capacity, nullptr);
    err_if(!_sem, "failed to create semaphore");
    _init_comlete.count_down();
  }

  void release(uint32_t count = 1) const noexcept
  {
    _init_comlete.wait();
    err_if(!ReleaseSemaphore(_sem, count, 0), "failed to release semaphore");
  }

  void destroy() const noexcept
  {
    _init_comlete.wait();
    err_if(!CloseHandle(_sem), "failed to destroy semaphore");
  }

  void acquire(uint32_t timeout = INFINITE) const noexcept
  {
    _init_comlete.wait();
    WaitForSingleObject(_sem, timeout);
  }

private:
  HANDLE     _sem{};
  std::latch _init_comlete{ 1 };
};

}
