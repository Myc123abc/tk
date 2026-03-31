#pragma once

#include <array>
#include <atomic>

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

  auto& data()   noexcept { return _buf.at(1 - _idx.load(std::memory_order_relaxed));                        }
  void  swap()   noexcept { _idx.store(1 - _idx.load(std::memory_order_relaxed), std::memory_order_release); }
  auto& access() noexcept { return _buf.at(_idx.load(std::memory_order_acquire));                            }

private:
  std::atomic_uint32_t _idx{};
  std::array<T, 2>     _buf{};
};

}
