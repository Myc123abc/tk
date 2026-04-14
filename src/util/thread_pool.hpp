#pragma once

#include "../config.hpp"

#include <thread>
#include <semaphore>
#include <vector>
#include <queue>
#include <functional>
#include <mutex>

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
    _exit.store(true, std::memory_order_release);
    _threads.clear();
  }

  // TODO: change to try_submit avoid block because all queues full
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
  void enqueue(std::function<void()> f) noexcept
  {
    // TODO: task dispatch algorithm
    _threads[0].enqueue(f);
  }

  struct Thread
  {
    Thread() noexcept
    {
      _mutex  = std::make_unique<std::mutex>();
      _sem    = std::make_unique<std::binary_semaphore>(0);
      _thread = std::thread(&Thread::main, this);
    }
    ~Thread() noexcept
    {
      _sem->release();
      _thread.join();
    }

    Thread(Thread&&) = default;

    void enqueue(std::function<void()> f) noexcept
    {
      std::lock_guard lock{ *_mutex };
      _tasks.emplace(f);
      _sem->release();
    }
  
  private:
    void main() noexcept
    {
      while (!instance()._exit.load(std::memory_order_acquire))
      {
        {
          std::lock_guard lock{ *_mutex };
          while (!_tasks.empty())
          {
            _tasks.front()();
            _tasks.pop();
          }
        }
        _sem->acquire();
      }
    }

  private:
    std::thread                            _thread;
    std::queue<std::function<void()>>      _tasks;
    std::unique_ptr<std::mutex>            _mutex;
    std::unique_ptr<std::binary_semaphore> _sem;
  };

private:
  std::atomic_bool          _exit;
  std::vector<Thread>       _threads;
};

inline static auto& g_thread_pool = ThreadPool::instance();

}
