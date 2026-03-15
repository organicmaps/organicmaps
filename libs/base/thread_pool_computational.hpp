#pragma once

#include "base/assert.hpp"
#include "base/thread_utils.hpp"

#include <functional>
#include <future>
#include <queue>
#include <thread>
#include <type_traits>

namespace base
{

// ComputationalThreadPool is needed for easy parallelization of tasks.
// ComputationalThreadPool can accept tasks that return result as std::future.
// When the destructor is called, all threads will join.
// Warning: ComputationalThreadPool works with std::thread instead of SimpleThread and therefore
// should not be used when the JVM is needed.
class ComputationalThreadPool
{
public:
  using FunctionType = threads::FunctionWrapper;
  using Threads = std::vector<std::thread>;

  // Constructs a ThreadPool.
  // threadCount - number of threads used by the thread pool.
  // Warning: The constructor may throw exceptions.
  explicit ComputationalThreadPool(size_t threadCount) : m_done(false), m_joiner(m_threads)
  {
    CHECK_GREATER(threadCount, 0, ());

    m_threads.reserve(threadCount);
    try
    {
      for (size_t i = 0; i < threadCount; i++)
        m_threads.emplace_back(&ComputationalThreadPool::Worker, this);
    }
    catch (...)  // std::system_error etc.
    {
      Stop();
      throw;
    }
  }

  // Destroys the ThreadPool.
  // This function will block until all runnables have been completed.
  ~ComputationalThreadPool()
  {
    {
      std::unique_lock lock(m_mutex);
      m_done = true;
    }
    m_condition.notify_all();
  }

  // Submit task for execution.
  // func - task to be performed.
  // args - arguments for func.
  // The function will return the object future.
  // Warning: If the thread pool is stopped then the call will be ignored.
  template <typename F, typename... Args>
  auto Submit(F && func, Args &&... args) -> std::future<std::invoke_result_t<std::decay_t<F>, std::decay_t<Args>...>>
  {
    using ResultType = std::invoke_result_t<std::decay_t<F>, std::decay_t<Args>...>;
    std::packaged_task<ResultType()> task([f = std::forward<F>(func), ... a = std::forward<Args>(args)]() mutable
    { return std::invoke(std::move(f), std::move(a)...); });
    std::future<ResultType> result(task.get_future());
    {
      std::unique_lock lock(m_mutex);
      if (m_done)
        return {};

      m_queue.emplace(std::move(task));
    }
    m_condition.notify_one();
    return result;
  }

  // Submit work for execution.
  // func - task to be performed.
  // args - arguments for func
  // Warning: If the thread pool is stopped then the call will be ignored.
  template <typename F, typename... Args>
  void SubmitWork(F && func, Args &&... args)
  {
    auto task = [f = std::forward<F>(func), ... a = std::forward<Args>(args)]() mutable
    { std::invoke(std::move(f), std::move(a)...); };
    {
      std::unique_lock lock(m_mutex);
      if (m_done)
        return;

      m_queue.emplace(std::move(task));
    }
    m_condition.notify_one();
  }

  // Stop a ThreadPool.
  // Removes the tasks that are not yet started from the queue.
  // Unlike the destructor, this function does not wait for all runnables to complete:
  // the tasks will stop as soon as possible.
  void Stop()
  {
    {
      std::unique_lock lock(m_mutex);
      auto empty = std::queue<FunctionType>();
      m_queue.swap(empty);
      m_done = true;
    }
    m_condition.notify_all();
  }

  void WaitingStop()
  {
    {
      std::unique_lock lock(m_mutex);
      m_done = true;
    }
    m_condition.notify_all();
    m_joiner.Join();
  }

private:
  void Worker()
  {
    while (true)
    {
      FunctionType task;
      {
        std::unique_lock lock(m_mutex);
        m_condition.wait(lock, [&] { return m_done || !m_queue.empty(); });

        if (m_done && m_queue.empty())
          return;

        // It may seem that at this point the queue may be empty, provided that m_done == false and
        // m_queue.empty() == true. But it is not possible that the queue is not empty guarantees
        // check in m_condition.wait.
        task = std::move(m_queue.front());
        m_queue.pop();
      }

      task();
    }
  }

  bool m_done;
  std::mutex m_mutex;
  std::condition_variable m_condition;
  std::queue<FunctionType> m_queue;
  Threads m_threads;
  threads::ThreadsJoiner<> m_joiner;
};

// Applies |func| to each element of |range| in parallel using |threadsCount| threads.
// Each element is copied into its task. The pool destructor waits for all tasks to finish.
template <typename Range, typename F>
void ParallelFor(size_t threadsCount, Range && range, F && func)
{
  ComputationalThreadPool pool(threadsCount);
  for (auto && item : range)
    pool.SubmitWork(func, item);
}

}  // namespace base
