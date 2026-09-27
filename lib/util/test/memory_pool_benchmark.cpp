#include "tk/memory_pool.hpp"

#include <algorithm>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <print>
#include <string_view>
#include <vector>

using namespace tk;

namespace {

constexpr auto Iterations = 1'000'000;
constexpr size_t Sizes[] = { 16_B, 32_B, 64_B, 128_B, 256_B, 512_B, 1_KiB, 2_KiB, 4_KiB };

struct Result
{
  std::string_view name;
  std::chrono::nanoseconds elapsed;
  std::uint64_t checksum;
};

template <typename F>
auto measure(std::string_view name, F&& func) -> Result
{
  auto start = std::chrono::steady_clock::now();
  auto checksum = func();
  auto end = std::chrono::steady_clock::now();
  return { name, std::chrono::duration_cast<std::chrono::nanoseconds>(end - start), checksum };
}

void touch(void* pointer, size_t size, std::uint64_t& checksum)
{
  auto bytes = static_cast<std::byte*>(pointer);
  bytes[0] = static_cast<std::byte>(size);
  bytes[size - 1] = static_cast<std::byte>(size >> 8);
  checksum += std::to_integer<unsigned char>(bytes[0]);
  checksum += std::to_integer<unsigned char>(bytes[size - 1]);
}

auto run_pool_bulk_benchmark() -> std::uint64_t
{
  MemoryPool<> pool;
  std::vector<void*> pointers;
  pointers.resize(Iterations);
  std::uint64_t checksum{};

  for (auto iteration = 0; iteration < Iterations; ++iteration)
  {
    auto size = Sizes[iteration % std::size(Sizes)];
    auto pointer = pool.alloc(size);
    touch(pointer, size, checksum);
    pointers[iteration] = pointer;
  }

  for (auto& pointer : pointers)
  {
    pool.free(pointer);
  }

  pool.destroy();
  return checksum;
}

auto run_heap_bulk_benchmark() -> std::uint64_t
{
  std::vector<void*> pointers;
  pointers.resize(Iterations);
  std::uint64_t checksum{};

  for (auto iteration = 0; iteration < Iterations; ++iteration)
  {
    auto size = Sizes[iteration % std::size(Sizes)];
    auto pointer = std::malloc(size);
    if (!pointer) std::abort();
    touch(pointer, size, checksum);
    pointers[iteration] = pointer;
  }

  for (auto pointer : pointers)
  {
    std::free(pointer);
  }

  return checksum;
}

auto run_pool_reuse_benchmark() -> std::uint64_t
{
  MemoryPool<> pool;
  std::uint64_t checksum{};

  for (auto iteration = 0; iteration < Iterations; ++iteration)
  {
    auto size = Sizes[iteration % std::size(Sizes)];
    auto pointer = pool.alloc(size);
    touch(pointer, size, checksum);
    pool.free(pointer);
  }

  pool.destroy();
  return checksum;
}

auto run_heap_reuse_benchmark() -> std::uint64_t
{
  std::uint64_t checksum{};

  for (auto iteration = 0; iteration < Iterations; ++iteration)
  {
    auto size = Sizes[iteration % std::size(Sizes)];
    auto pointer = std::malloc(size);
    if (!pointer) std::abort();
    touch(pointer, size, checksum);
    std::free(pointer);
  }

  return checksum;
}

void print_result(Result result)
{
  auto ms = std::chrono::duration<double, std::milli>(result.elapsed).count();
  auto ns_per_operation = static_cast<double>(result.elapsed.count()) / Iterations;
  std::println("{:<12} {:>10.3f} ms {:>10.2f} ns/op checksum={}", result.name, ms, ns_per_operation, result.checksum);
}

}

auto main() -> int
{
  auto pool_bulk = measure("pool bulk", run_pool_bulk_benchmark);
  auto heap_bulk = measure("malloc bulk", run_heap_bulk_benchmark);
  auto pool_reuse = measure("pool reuse", run_pool_reuse_benchmark);
  auto heap_reuse = measure("malloc/free", run_heap_reuse_benchmark);

  print_result(pool_bulk);
  print_result(heap_bulk);
  print_result(pool_reuse);
  print_result(heap_reuse);
}
