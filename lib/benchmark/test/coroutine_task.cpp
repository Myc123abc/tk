#include "async/thread_pool.hpp"

#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <string_view>
#include <thread>
#include <vector>

namespace {

constexpr auto Trial_Count            = 7u;
constexpr auto Coroutine_Count        = 250'000u;
constexpr auto Completion_Poll_Count  = 10'000'000u;
constexpr auto Thread_Pool_Wait_Count = 10'000u;
constexpr auto Thread_Pool_Batch_Count = 100'000u;

std::atomic<uint64_t> benchmark_sink{};

auto immediate_task(uint64_t value) -> tk::Task<uint64_t>
{
  co_return value + 1;
}

auto completed_task_chain(uint64_t value) -> tk::Task<uint64_t>
{
  co_return co_await immediate_task(value);
}

auto thread_pool_task_chain(uint64_t value) -> tk::Task<uint64_t>
{
  co_return co_await tk::g_thread_pool.run([value] { return value + 1; });
}

auto immediate_tasks(uint32_t count) -> uint64_t
{
  auto sum = uint64_t{};
  for (auto index = 0u; index < count; ++index)
    sum += immediate_task(index).take_result();
  return sum;
}

auto completed_task_chains(uint32_t count) -> uint64_t
{
  auto sum = uint64_t{};
  for (auto index = 0u; index < count; ++index)
    sum += completed_task_chain(index).take_result();
  return sum;
}

auto completed_task_polls(uint32_t count) -> uint64_t
{
  auto task = immediate_task(0);
  auto completed_count = uint64_t{};
  for (auto index = 0u; index < count; ++index)
    completed_count += task.is_completed();
  return completed_count + task.take_result();
}

auto thread_pool_waits(uint32_t count) -> uint64_t
{
  auto sum = uint64_t{};
  for (auto index = 0u; index < count; ++index)
    sum += tk::g_thread_pool.run([index] { return static_cast<uint64_t>(index) + 1; }).take_result();
  return sum;
}

auto thread_pool_task_chains(uint32_t count) -> uint64_t
{
  auto sum = uint64_t{};
  for (auto index = 0u; index < count; ++index)
    sum += thread_pool_task_chain(index).take_result();
  return sum;
}

auto thread_pool_batch(uint32_t count) -> uint64_t
{
  auto tasks = std::vector<tk::Task<uint64_t>>{};
  tasks.reserve(count);
  for (auto index = 0u; index < count; ++index)
    tasks.emplace_back(tk::g_thread_pool.run([index] { return static_cast<uint64_t>(index) + 1; }));

  auto sum = uint64_t{};
  for (auto const& task : tasks)
    sum += task.take_result();
  return sum;
}

auto median_ns_per_operation(uint32_t count, auto&& func) -> double
{
  benchmark_sink.fetch_xor(func(std::max(1u, count / 10)), std::memory_order_relaxed);

  auto trials = std::array<double, Trial_Count>{};
  for (auto& trial : trials)
  {
    auto const begin = std::chrono::steady_clock::now();
    auto const result = func(count);
    auto const end = std::chrono::steady_clock::now();
    benchmark_sink.fetch_xor(result, std::memory_order_relaxed);
    trial = std::chrono::duration<double, std::nano>(end - begin).count() / count;
  }
  std::ranges::sort(trials);
  return trials[Trial_Count / 2];
}

void print_result(std::string_view name, double ns_per_operation)
{
  auto const million_operations_per_second = 1'000. / ns_per_operation;
  std::cout
    << std::left  << std::setw(34) << name
    << std::right << std::setw(14) << std::fixed << std::setprecision(2) << ns_per_operation
    << std::setw(14) << std::setprecision(3) << million_operations_per_second
    << '\n';
}

}

int main()
{
  auto const hardware_threads = std::max(1u, std::thread::hardware_concurrency());
  auto const worker_count = std::max(1u, hardware_threads - 1);
  tk::g_thread_pool.init(static_cast<int>(worker_count));

  std::cout
    << "workers: " << worker_count << '\n'
    << std::left  << std::setw(34) << "benchmark"
    << std::right << std::setw(14) << "ns/op"
    << std::setw(14) << "M ops/s"
    << '\n';

  print_result("Task<T> immediate", median_ns_per_operation(Coroutine_Count, immediate_tasks));
  print_result("Task<T> completed co_await", median_ns_per_operation(Coroutine_Count, completed_task_chains));
  print_result("Task<T> is_completed poll", median_ns_per_operation(Completion_Poll_Count, completed_task_polls));
  print_result("ThreadPool run + wait", median_ns_per_operation(Thread_Pool_Wait_Count, thread_pool_waits));
  print_result("ThreadPool co_await chain", median_ns_per_operation(Thread_Pool_Wait_Count, thread_pool_task_chains));
  print_result("ThreadPool batched", median_ns_per_operation(Thread_Pool_Batch_Count, thread_pool_batch));

  tk::g_thread_pool.destroy();
  return benchmark_sink.load(std::memory_order_relaxed) == 0;
}