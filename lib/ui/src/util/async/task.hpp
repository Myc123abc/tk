#pragma once

#include <atomic>
#include <coroutine>
#include <optional>
#include <mutex>
#include <condition_variable>
#include <assert.h>

#include "tk/error_handling.hpp"

namespace tk {

template <typename T>
struct PromiseReturn
{
  std::optional<T> result;

  template <typename U>
  requires std::convertible_to<U, T>
  void return_value(U&& value)
  {
    result.emplace(std::forward<U>(value));
  }
};

template<>
struct PromiseReturn<void>
{
  void return_void() noexcept {}
};

template <typename T = void>
class Task
{
public:
  Task()                       = default;
  Task(Task const&)            = delete;
  Task& operator=(Task const&) = delete;

  Task(Task&& task) noexcept
    : _handle(std::exchange(task._handle, {})) {}
  
  auto operator=(Task&& task) noexcept -> Task&
  {
    if (this != &task)
    {
      reset();
      _handle = std::exchange(task._handle, {});
    }
    return *this;
  }

  void reset() noexcept
  {
    if (_handle)
    {
      err_if(!_handle.done(), "failed to reset Task because of uncompleted coroutine");
      _handle.destroy();
      _handle = {};
    }
  }

  struct promise_type;
  using Handle = std::coroutine_handle<promise_type>;

  Task(Handle handle) noexcept : _handle(handle) {}
  ~Task() noexcept { reset(); }

  struct promise_type : PromiseReturn<T>
  {
    auto get_return_object() noexcept -> Task { return Handle::from_promise(*this); }
    auto initial_suspend() const noexcept -> std::suspend_never { return {}; }
    auto final_suspend() const noexcept { return FinalAwaiter{}; }
    void unhandled_exception() const noexcept { exit("failed to run coroutine which happened exception."); }

    std::mutex              mutex;
    std::condition_variable cv;
    std::coroutine_handle<> continuation{};
    std::atomic_bool        completed;
  };

  struct Awaiter
  {
    Handle handle{};

    auto await_ready() const noexcept
    {
      return handle.promise().completed.load(std::memory_order_acquire);
    }

    auto await_suspend(std::coroutine_handle<> h) noexcept
    {
      auto& promise = handle.promise();
      auto  lock    = std::lock_guard{ promise.mutex };

      if (promise.completed.load(std::memory_order_acquire)) return false;

      assert(!promise.continuation);
      promise.continuation = h;
      return true;
    }

    auto await_resume() const noexcept -> T
    {
      auto& promise = handle.promise();
      if constexpr (!std::is_void_v<T>)
      {
        assert(promise.result);
        auto value = std::move(*promise.result);
        promise.result.reset();
        return value;
      }
    }
  };

  auto operator co_await() noexcept
  {
    return Awaiter{ _handle };
  }

  auto is_completed() const noexcept -> bool
  {
    assert(_handle);
    return _handle.promise().completed.load(std::memory_order_acquire);
  }

  struct FinalAwaiter
  {
    auto await_ready() const noexcept { return false; }

    auto await_suspend(Handle h) const noexcept -> std::coroutine_handle<>
    {
      auto& promise = h.promise();
      auto  handle  = std::coroutine_handle<>{};
      {
        auto lock = std::lock_guard{ promise.mutex };
        promise.completed.store(true, std::memory_order_release);
        handle = std::exchange(promise.continuation, {});
      }
      promise.cv.notify_all();
      return handle ? handle : std::noop_coroutine();
    }

    void await_resume() const noexcept {}
  };

  void wait() const noexcept
  {
    assert(_handle);
    auto& promise = _handle.promise();
    auto  lock    = std::unique_lock{ promise.mutex };
    promise.cv.wait(lock, [&] { return promise.completed.load(std::memory_order_acquire); });
  }

  auto take_result() const noexcept -> T
  {
    wait();
    auto& promise = _handle.promise();
    if constexpr (!std::is_void_v<T>)
    {
      assert(promise.result);
      auto value = std::move(*promise.result);
      promise.result.reset();
      return value;
    }
  }

private:
  Handle _handle{};
};

}
