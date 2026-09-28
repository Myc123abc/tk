#include "tk/benchmark.hpp"

#include <vector>
#include <print>
#include <chrono>
#include <functional>
#include <ranges>

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

struct Result
{
  long long alloc_time{};
  long long free_time{};
};

auto heap_bulk_alloc(int size, int cnt) noexcept -> Result
{
  auto ps = std::vector<void*>(cnt);

  auto alloc_time = measure([&]
  {
    for (auto& p : ps) p = malloc(size);
  });

  auto free_time = measure([&]
  {
    for (auto p : ps) free(p);
  });

  return { alloc_time, free_time };
}

auto main() -> int
{
  // get alloc test sizes
  auto constexpr max_size = 65536;
  auto constexpr test_cnt = 65536;
  auto alloc_sizes = std::vector<float>(std::log2(max_size));
  for (auto [i, size] : alloc_sizes | std::views::enumerate)
    size = std::pow(2, i + 1);

  // get test results of heap bulk alloc
  auto results = std::vector<Result>{};
  results.reserve(alloc_sizes.size());
  for (auto size : alloc_sizes)
    results.emplace_back(heap_bulk_alloc(size, test_cnt));

  auto to_vec = [](auto&& range, auto member)
  {
    return range | std::views::transform(member) | std::ranges::to<std::vector<float>>();
  };

  auto alloc_results = to_vec(results, &Result::alloc_time);
  auto free_results  = to_vec(results, &Result::free_time);

  tk::benchmark::init();
  tk::benchmark::set_labels("size (B)", "Time (us)");

  while (tk::benchmark::running())
  {
    tk::benchmark::present(alloc_sizes, alloc_results, 0x0000ffff);
    tk::benchmark::present(alloc_sizes, free_results, 0x00ff00ff);
    tk::benchmark::update();
  }

  tk::benchmark::destroy();
}
