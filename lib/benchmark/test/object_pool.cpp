#include "tk/benchmark.hpp"

#include "object_pool.hpp"

#include <array>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <limits>
#include <memory>
#include <optional>
#include <ranges>
#include <span>
#include <vector>

using ns = std::chrono::nanoseconds;

constexpr auto Trial_Count = 9;

struct BenchmarkObject
{
  uint64_t                  value{};
  std::array<std::byte, 56> padding{};
};

using Pool = tk::ObjectPool<BenchmarkObject>;

template <typename TimeUnit = ns>
auto measure(auto&& func) noexcept
{
  auto const begin = std::chrono::steady_clock::now();
  func();
  auto const end = std::chrono::steady_clock::now();
  return std::chrono::duration_cast<TimeUnit>(end - begin).count();
}

auto median_time(auto&& func) noexcept
{
  func();

  auto trials = std::array<int64_t, Trial_Count>{};
  for (auto& trial : trials)
    trial = func();
  std::ranges::sort(trials);
  return static_cast<float>(trials[Trial_Count / 2]);
}

auto pool_allocate(uint32_t count) noexcept
{
  auto pool    = std::optional<Pool>{};
  auto handles = std::vector<Pool::Handle>{};
  handles.reserve(count);

  auto const elapsed = measure([&]
  {
    pool.emplace();
    for ([[maybe_unused]] auto index : std::views::iota(0u, count))
      handles.emplace_back(pool->alloc());
  });
  for (auto& handle : handles)
    pool->free(handle);
  return elapsed;
}

auto heap_allocate(uint32_t count) noexcept
{
  auto objects = std::vector<BenchmarkObject*>{};
  objects.reserve(count);

  auto const elapsed = measure([&]
  {
    for ([[maybe_unused]] auto index : std::views::iota(0u, count))
      objects.emplace_back(new BenchmarkObject{});
  });
  for (auto* object : objects)
    delete object;
  return elapsed;
}

auto pool_get(uint32_t count) noexcept
{
  auto pool    = Pool{};
  auto handles = std::vector<Pool::Handle>{};
  handles.reserve(count);
  for (auto index : std::views::iota(0u, count))
  {
    auto handle = pool.alloc();
    pool[handle].value = index;
    handles.emplace_back(handle);
  }

  auto sum = uint64_t{};
  auto const elapsed = measure([&]
  {
    for (auto handle : handles)
      sum += pool[handle].value;
  });
  for (auto& handle : handles)
    pool.free(handle);
  return elapsed + static_cast<int64_t>(sum == std::numeric_limits<uint64_t>::max());
}

auto pointer_get(uint32_t count) noexcept
{
  auto objects = std::vector<std::unique_ptr<BenchmarkObject>>{};
  objects.reserve(count);
  for (auto index : std::views::iota(0u, count))
    objects.emplace_back(std::make_unique<BenchmarkObject>(index));

  auto sum = uint64_t{};
  auto const elapsed = measure([&]
  {
    for (auto const& object : objects)
      sum += object->value;
  });
  return elapsed + static_cast<int64_t>(sum == std::numeric_limits<uint64_t>::max());
}

auto pool_free(uint32_t count) noexcept
{
  auto pool    = Pool{};
  auto handles = std::vector<Pool::Handle>{};
  handles.reserve(count);
  for ([[maybe_unused]] auto index : std::views::iota(0u, count))
    handles.emplace_back(pool.alloc());
  return measure([&] { for (auto& handle : handles) pool.free(handle); });
}

auto heap_free(uint32_t count) noexcept
{
  auto objects = std::vector<BenchmarkObject*>{};
  objects.reserve(count);
  for ([[maybe_unused]] auto index : std::views::iota(0u, count))
    objects.emplace_back(new BenchmarkObject{});
  return measure([&] { for (auto* object : objects) delete object; });
}

auto pool_reuse(uint32_t count) noexcept
{
  auto pool    = Pool{};
  auto handles = std::vector<Pool::Handle>{};
  handles.reserve(count);
  for ([[maybe_unused]] auto index : std::views::iota(0u, count))
    handles.emplace_back(pool.alloc());
  for (auto& handle : handles)
    pool.free(handle);
  handles.clear();

  auto const elapsed = measure([&]
  {
    for ([[maybe_unused]] auto index : std::views::iota(0u, count))
      handles.emplace_back(pool.alloc());
    for (auto& handle : handles)
      pool.free(handle);
  });
  return elapsed;
}

auto heap_reuse(uint32_t count) noexcept
{
  auto objects = std::vector<BenchmarkObject*>{};
  objects.reserve(count);
  for ([[maybe_unused]] auto index : std::views::iota(0u, count))
    objects.emplace_back(new BenchmarkObject{});
  for (auto* object : objects)
    delete object;
  objects.clear();

  return measure([&]
  {
    for ([[maybe_unused]] auto index : std::views::iota(0u, count))
      objects.emplace_back(new BenchmarkObject{});
    for (auto* object : objects)
      delete object;
  });
}

auto test(std::span<float const> counts, auto&& func)
{
  auto results = std::vector<float>{};
  results.reserve(counts.size());
  for (auto count : counts)
    results.emplace_back(median_time([&] { return func(static_cast<uint32_t>(count)); }) / count);
  return results;
}

void verify_address_stability() noexcept
{
  auto pool = Pool{};
  auto handles = std::vector<Pool::Handle>{};
  handles.reserve(65);
  handles.emplace_back(pool.alloc());
  auto* first = &pool[handles.front()];
  for ([[maybe_unused]] auto index : std::views::iota(1, 65))
    handles.emplace_back(pool.alloc());
  assert(&pool[handles.front()] == first);
  for (auto& handle : handles)
    pool.free(handle);
}

auto main() -> int
{
  verify_address_stability();

  constexpr auto Max_Count = 65'536;
  auto counts = std::vector<float>(std::log2(Max_Count));
  for (auto [index, count] : counts | std::views::enumerate)
    count = std::pow(2, index + 1);

  auto pool_allocate_results = test(counts, pool_allocate);
  auto heap_allocate_results = test(counts, heap_allocate);
  auto pool_get_results      = test(counts, pool_get);
  auto pointer_get_results   = test(counts, pointer_get);
  auto pool_free_results     = test(counts, pool_free);
  auto heap_free_results     = test(counts, heap_free);
  auto pool_reuse_results    = test(counts, pool_reuse);
  auto heap_reuse_results    = test(counts, heap_reuse);

  tk::benchmark::init();
  tk::benchmark::set_labels("Object count", "Time per object (ns)");
  tk::benchmark::limit_size(450, 450);

  while (tk::benchmark::running())
  {
    tk::benchmark::present(counts, pool_allocate_results, 0x00ff00ff, "pool allocate");
    tk::benchmark::present(counts, heap_allocate_results, 0xff8800ff, "heap allocate");
    tk::benchmark::present(counts, pool_get_results,      0x00ffffff, "pool get");
    tk::benchmark::present(counts, pointer_get_results,   0xffff00ff, "pointer get");
    tk::benchmark::present(counts, pool_free_results,     0x0088ffff, "pool free");
    tk::benchmark::present(counts, heap_free_results,     0xff00ffff, "heap free");
    tk::benchmark::present(counts, pool_reuse_results,    0xffffffff, "pool reuse cycle");
    tk::benchmark::present(counts, heap_reuse_results,    0xff0000ff, "heap reuse cycle");
    tk::benchmark::update();
  }

  tk::benchmark::destroy();
}
