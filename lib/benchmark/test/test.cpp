#include "tk/benchmark.hpp"

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <limits>
#include <unordered_map>
#include <vector>
#include <ranges>

using ns = std::chrono::nanoseconds;
using us = std::chrono::microseconds;
using ms = std::chrono::milliseconds;

auto constexpr TRIAL_COUNT = 9;

template <typename TimeUnit = us>
auto measure(auto&& f) noexcept
{
  auto beg = std::chrono::steady_clock::now();
  f();
  auto end = std::chrono::steady_clock::now();
  return std::chrono::duration_cast<TimeUnit>(end - beg).count();
}

auto median_time(auto&& f) noexcept
{
  f();

  auto trials = std::array<int64_t, TRIAL_COUNT>{};
  for (auto& trial : trials)
    trial = f();
  std::ranges::sort(trials);
  return static_cast<float>(trials[TRIAL_COUNT / 2]);
}

auto map_insert(uint32_t num) noexcept
{
  auto map = std::unordered_map<uint32_t, uint32_t>{};
  return measure([&] { for (auto i : std::views::iota(0u, num)) map.emplace(i, 0); });
}

auto map_insert_reserve(uint32_t num) noexcept
{
  auto map = std::unordered_map<uint32_t, uint32_t>{};
  map.reserve(num);
  return measure([&] { for (auto i : std::views::iota(0u, num)) map.emplace(i, 0); });
}

auto map_remove(uint32_t num) noexcept
{
  auto map = std::unordered_map<uint32_t, uint32_t>{};
  map.reserve(num);
  for (auto i : std::views::iota(0u, num))
    map.emplace(i, 0);
  return measure([&] { for (auto i : std::views::iota(0u, num)) map.erase(i); });
}

auto map_find(uint32_t num) noexcept
{
  auto map = std::unordered_map<uint32_t, uint32_t>{};
  map.reserve(num);
  for (auto i : std::views::iota(0u, num))
    map.emplace(i, i);

  auto sum = uint64_t{};
  auto elapsed = measure([&]
  {
    for (auto i : std::views::iota(0u, num))
      if (auto it = map.find(i); it != map.end())
        sum += it->second;
  });
  return elapsed + static_cast<int64_t>(sum == std::numeric_limits<uint64_t>::max());
}

auto map_find_missing(uint32_t num) noexcept
{
  auto map = std::unordered_map<uint32_t, uint32_t>{};
  map.reserve(num);
  for (auto i : std::views::iota(0u, num))
    map.emplace(i, i);

  auto found_count = uint32_t{};
  auto elapsed = measure([&]
  {
    for (auto i : std::views::iota(num, num * 2))
      found_count += map.find(i) != map.end();
  });
  return elapsed + static_cast<int64_t>(found_count == num);
}

auto map_mixed(uint32_t num) noexcept
{
  auto map = std::unordered_map<uint32_t, uint32_t>{};
  map.reserve(num);

  auto sum = uint64_t{};
  auto elapsed = measure([&]
  {
    for (auto i : std::views::iota(0u, num))
    {
      map.emplace(i, i);
      if (auto it = map.find(i / 2); it != map.end())
        sum += it->second;
      if ((i % 4) == 3)
        map.erase(i - 3);
    }
  });
  return elapsed + static_cast<int64_t>(sum == std::numeric_limits<uint64_t>::max());
}

auto test(std::span<float> numbers, auto&& func) noexcept
{
  auto results = std::vector<float>{};
  results.reserve(numbers.size());
  for (auto num : numbers)
    results.emplace_back(median_time([&] { return func(static_cast<uint32_t>(num)); }));
  return results;
}

auto main() -> int
{
  // get test numbers
  auto constexpr max_size = 65536;
  auto numbers = std::vector<float>(std::log2(max_size));
  for (auto [i, size] : numbers | std::views::enumerate)
    size = std::pow(2, i + 1);

  // save test results
  auto insert_results         = test(numbers, map_insert);
  auto insert_reserve_results = test(numbers, map_insert_reserve);
  auto remove_results         = test(numbers, map_remove);
  auto find_results           = test(numbers, map_find);
  auto find_missing_results   = test(numbers, map_find_missing);
  auto mixed_results          = test(numbers, map_mixed);

  tk::benchmark::init();
  tk::benchmark::set_labels("Number", "Time (us)");
  tk::benchmark::limit_size(450, 450);

  while (tk::benchmark::running())
  {
    tk::benchmark::present(numbers, insert_results,         0x0000ffff, "insert");
    tk::benchmark::present(numbers, insert_reserve_results, 0x00ff00ff, "insert (reserve)");
    tk::benchmark::present(numbers, remove_results,         0x00ffffff, "remove");
    tk::benchmark::present(numbers, find_results,           0xffff00ff, "find");
    tk::benchmark::present(numbers, find_missing_results,   0xff00ffff, "find missing");
    tk::benchmark::present(numbers, mixed_results,          0xff8800ff, "mixed");
    tk::benchmark::update();
  }

  tk::benchmark::destroy();
}
