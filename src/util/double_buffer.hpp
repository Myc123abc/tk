#pragma once

#include <array>
#include "util/base.hpp"
// #include <atomic>

namespace tk {

template <typename T>
class DoubleBuffer
{
public:
  DoubleBuffer()                                   = default;
  ~DoubleBuffer()                                  = default;
  DoubleBuffer(DoubleBuffer const&)                = delete;
  DoubleBuffer(DoubleBuffer&&) noexcept            = delete;
  DoubleBuffer& operator=(DoubleBuffer const&)     = delete;
  DoubleBuffer& operator=(DoubleBuffer&&) noexcept = delete;

  // auto& data()   noexcept { return _buf[1 - _idx.load(std::memory_order_relaxed)];                           }
  // void  swap()   noexcept { _idx.store(1 - _idx.load(std::memory_order_relaxed), std::memory_order_release); }
  // auto& access() noexcept { return _buf[_idx.load(std::memory_order_acquire)];                               }

  auto& data()   noexcept { return _buf[_idx];     }
  void  swap()   noexcept { _idx = 1 - _idx;       }
  auto& access() noexcept { return _buf[1 - _idx]; }

private:
  std::array<T, 2> _buf{};
  uint             _idx{};
  // std::atomic_uint _idx{};
};

}
