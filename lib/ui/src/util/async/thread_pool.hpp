#pragma once

#include <thread>
#include <vector>
#include <queue>
#include <functional>
#include <mutex>
#include <cassert>

#include "task.hpp"

namespace tk {

class ThreadPool
{
public:
  static auto& instance() noexcept
  {
    static ThreadPool instance;
    return instance;
  }

  auto size() const noexcept { return _threads.size(); }

  void init(int size) noexcept
  {
    assert(_threads.empty());
    assert(_tasks.empty());
    _exit.store(false, std::memory_order_release);
    _threads.resize(size);
  }

  void destroy() noexcept
  {
    {
      auto lock = std::lock_guard{ _mutex };
      _exit.store(true, std::memory_order_release);
    }
    _cv.notify_all();
    _threads.clear();
    assert(_tasks.empty());
  }

  template <typename Func>
  [[nodiscard]] auto run(Func func) noexcept -> Task<std::invoke_result_t<Func&>>
  {
    using Result = std::invoke_result_t<Func&>;

    struct Awaiter
    {
      ThreadPool& pool;
    
      auto await_ready() const noexcept { return false; }
      void await_suspend(std::coroutine_handle<> h) noexcept
      {
        pool.enqueue([h] { h.resume(); });
      }
      void await_resume() const noexcept {}
    };
    co_await Awaiter{ *this };

    if constexpr (std::is_void_v<Result>)
      std::invoke(func);
    else
      co_return std::invoke(func);
  }

  template <typename Func>
  void submit(Func func) noexcept
  {
    enqueue([func = std::move(func)] mutable { std::invoke(func); });
  }

private:
  struct Thread
  {
    Thread() noexcept
    {
      _thread = std::thread(&Thread::main, this);
    }

    ~Thread() noexcept
    {
      _thread.join();
    }

    Thread(Thread&&) = default;

  private:
    void main() noexcept
    {
      auto& pool = instance();
      while (true)
      {
        auto task = std::move_only_function<void()>{};

        {
          std::unique_lock lock{ pool._mutex };
          pool._cv.wait(lock, [&]{
            return pool._exit.load(std::memory_order_acquire) ||
                  !pool._tasks.empty();
          });

          if (pool._tasks.empty())
          {
            assert(pool._exit.load(std::memory_order_acquire));
            return;
          }

          task = std::move(pool._tasks.front());
          pool._tasks.pop();
        }

        task();
      }
    }

  private:
    std::thread _thread;
  };

  void enqueue(std::move_only_function<void()> f) noexcept
  {
    {
      auto lock = std::lock_guard{ _mutex };
      err_if(_exit.load(std::memory_order_acquire),
        "failed to enqueue task because the thread pool is stopped");
      _tasks.emplace(std::move(f));
    }
    _cv.notify_one();
  }

private:
  std::atomic_bool                            _exit{ true };
  std::vector<Thread>                         _threads;
  std::queue<std::move_only_function<void()>> _tasks;
  std::mutex                                  _mutex;
  std::condition_variable                     _cv;
};

inline static auto& g_thread_pool = ThreadPool::instance();

}
