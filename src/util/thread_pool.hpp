#pragma once

#include "../config.hpp"

#include <thread>
#include <vector>
#include <queue>
#include <functional>
#include <mutex>
#include <algorithm>
#include <cassert>

namespace tk {

template <typename T>
struct Task
{
  auto is_completed() const noexcept { return _impl->complete.load(std::memory_order_acquire); }
  auto take_result() noexcept
  {
    assert(is_completed());
    return std::move(_impl->result);
  }

private:
  struct Impl
  {
    T                result;
    std::atomic_bool complete;
  };
  std::shared_ptr<Impl> _impl;

  friend class ThreadPool;

  void complete() noexcept { _impl->complete.store(true, std::memory_order_release); }
  void set_result(T&& v) noexcept { _impl->result = std::move(v); }
};

class ThreadPool
{
public:
  static auto& instance() noexcept
  {
    static ThreadPool instance;
    return instance;
  }

  void init(int size = 0) noexcept
  {
    if (size < Thread_Pool_Min_Size)
      size = std::max(static_cast<int>(std::thread::hardware_concurrency() - 1 - Used_Thread_Num), Thread_Pool_Min_Size);
    _threads.resize(size);
  }

  void destroy() noexcept
  {
    {
      std::lock_guard lock(_mutex);
      while (!_tasks.empty()) _tasks.pop();
    }
    _exit.store(true, std::memory_order_release);
    _cv.notify_all();
    _threads.clear();
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
      while (!pool._exit.load(std::memory_order_acquire))
      {
        auto task = std::function<void()>{};

        {
          std::unique_lock lock{ pool._mutex };
          pool._cv.wait(lock, [&]{
            return pool._exit.load(std::memory_order_acquire) ||
                  !pool._tasks.empty();
          });

          if (pool._exit && pool._tasks.empty())
            return;

          task = std::move(pool._tasks.front());
          pool._tasks.pop();
        }

        task();
      }
    }

  private:
    std::thread _thread;
  };

private:
  void enqueue(std::function<void()> f) noexcept
  {
    assert(!_exit.load(std::memory_order_acquire));
    {
      std::lock_guard lock(_mutex);
      _tasks.emplace(std::move(f));
    }
    _cv.notify_one();
  }

public:
  template <typename Func>
  auto submit(Func&& f) noexcept
  {
    using T = std::invoke_result_t<Func>;
 
    auto task = Task<T>{};
    task._impl = std::make_shared<typename Task<T>::Impl>();

    enqueue([task, f = std::forward<Func>(f)] mutable
    {
      if constexpr (std::is_void_v<T>)
        f();
      else
        task.set_result(f());
      task.complete();
    });

    return task;
  }

private:
  std::atomic_bool                  _exit;
  std::vector<Thread>               _threads;
  std::queue<std::function<void()>> _tasks;
  std::mutex                        _mutex;
  std::condition_variable           _cv;
};

inline static auto& g_thread_pool = ThreadPool::instance();

}
