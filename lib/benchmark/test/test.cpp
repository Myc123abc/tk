#include "tk/benchmark.hpp"

#include <vector>
#include <memory>
#include <print>
#include <chrono>
#include <functional>

using ns = std::chrono::nanoseconds;
using us = std::chrono::microseconds;
using ms = std::chrono::milliseconds;

template <typename TimeUnit = us>
auto measure(std::function<void()> f) noexcept
{
  auto beg = std::chrono::steady_clock::now();
  f();
  auto end = std::chrono::steady_clock::now();
  return std::chrono::duration_cast<TimeUnit>(end - beg).count();
}

auto heap_bulk_alloc(int size, int cnt) noexcept
{
  auto ps = std::vector<void*>(cnt);

  auto malloc_dur = measure([&]
  {
    for (auto& p : ps) p = malloc(size);
  });

  auto free_dur = measure([&]
  {
    for (auto p : ps) free(p);
  });

  std::println(
    "heap alloc consume: (size : {}, count: {})\n"
    "alloc: {}ns\n"
    "free:  {}ns"
    , size, cnt, malloc_dur, free_dur
  );
}

auto main() -> int
{
  heap_bulk_alloc(64, 1024);

  tk::benchmark::test();
}
