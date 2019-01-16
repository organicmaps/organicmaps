// This file contains PrimitiveThreadPool class.
#pragma once

#include "base/assert.hpp"
#include "base/thread_utils.hpp"

#include <atomic>
#include <functional>
#include <future>
#include <memory>
#include <queue>
#include <thread>

namespace threads
{
// PrimitiveThreadPool is needed for easy parallelization of tasks.
// PrimitiveThreadPool can accept tasks that return result as std::future.
// When the destructor is called, all threads will join.
//
// Usage example:
// size_t threadCount = 4;
// size_t counter = 0;
// {
//   std::mutex mutex;
//   threads::PrimitiveThreadPool threadPool(threadCount);
//   for (size_t i = 0; i < threadCount; ++i)
//   {
//     threadPool.Submit([&]() {
//       std::this_thread::sleep_for(std::chrono::milliseconds(1));
//       std::lock_guard<std::mutex> lock(mutex);
//       ++counter;
//     });
//   }
// }
// TEST_EQUAL(threadCount, counter, ());
//
class PrimitiveThreadPool
{
public:
  using FuntionType = FunctionWrapper;
  using Threads = std::vector<std::thread>;

  PrimitiveThreadPool(size_t threadCount) : m_done(false), m_joiner(m_threads)
  {
    CHECK_GREATER(threadCount, 0, ());

    for (size_t i = 0; i < threadCount; i++)
      m_threads.push_back(std::thread(&PrimitiveThreadPool::Worker, this));
  }

  ~PrimitiveThreadPool()
  {
    {
      std::unique_lock<std::mutex> lock(m_mutex);
      m_done = true;
    }
    m_condition.notify_all();
  }

  template<typename F, typename... Args>
  auto Submit(F && func, Args &&... args) ->std::future<decltype(func(args...))>
  {
    using ResultType = decltype(func(args...));
    std::packaged_task<ResultType()> task(std::bind(std::forward<F>(func),
                                                    std::forward<Args>(args)...));
    std::future<ResultType> result(task.get_future());
    {
      std::unique_lock<std::mutex> lock(m_mutex);
      m_queue.push(std::move(task));
    }
    m_condition.notify_one();
    return result;
  }

private:
  void Worker()
  {
    while (true)
    {
      FuntionType task;
      {
        std::unique_lock<std::mutex> lock(m_mutex);
        m_condition.wait(lock, [&] {
          return m_done || !m_queue.empty();
        });

        if (m_done && m_queue.empty())
          return;

        task = std::move(m_queue.front());
        m_queue.pop();
      }

      task();
    }
  }

  bool m_done;
  std::mutex m_mutex;
  std::condition_variable m_condition;
  std::queue<FuntionType> m_queue;
  Threads m_threads;
  StandartThreadsJoiner m_joiner;
};
}  // namespace threads
