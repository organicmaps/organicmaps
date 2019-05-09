#pragma once

#include "base/assert.hpp"
#include "base/task_loop.hpp"
#include "base/thread.hpp"
#include "base/thread_checker.hpp"

#include <chrono>
#include <condition_variable>
#include <mutex>
#include <queue>
#include <utility>
#include <vector>

namespace base
{
namespace thread_pool
{
namespace delayed
{
// This class represents a simple thread pool with a queue of tasks.
//
// *NOTE* This class IS NOT thread-safe, it must be destroyed on the
// same thread it was created, but Push* methods are thread-safe.
class ThreadPool : public TaskLoop
{
public:
  using Clock = std::chrono::steady_clock;
  using Duration = Clock::duration;
  using TimePoint = Clock::time_point;

  enum class Exit
  {
    ExecPending,
    SkipPending
  };

  explicit ThreadPool(size_t threadsCount = 1, Exit e = Exit::SkipPending);
  ~ThreadPool() override;

  // Pushes task to the end of the thread's queue of immediate tasks.
  // Returns false when the thread is shut down.
  //
  // The task |t| is going to be executed after all immediate tasks
  // that were pushed pushed before it.
  bool Push(Task && t) override;
  bool Push(Task const & t) override;

  // Pushes task to the thread's queue of delayed tasks. Returns false
  // when the thread is shut down.
  //
  // The task |t| is going to be executed not earlier than after
  // |delay|.  No other guarantees about execution order are made.  In
  // particular, when executing:
  //
  // PushDelayed(3ms, task1);
  // PushDelayed(1ms, task2);
  //
  // there is no guarantee that |task2| will be executed before |task1|.
  //
  // NOTE: current implementation depends on the fact that
  // steady_clock is the same for different threads.
  bool PushDelayed(Duration const & delay, Task && t);
  bool PushDelayed(Duration const & delay, Task const & t);

  // Sends a signal to the thread to shut down. Returns false when the
  // thread was shut down previously.
  bool Shutdown(Exit e);

  // Sends a signal to the thread to shut down and waits for completion.
  void ShutdownAndJoin();

  static TimePoint Now() { return Clock::now(); }

private:
  enum QueueType
  {
    QUEUE_TYPE_IMMEDIATE,
    QUEUE_TYPE_DELAYED,
    QUEUE_TYPE_COUNT
  };

  struct DelayedTask
  {
    template <typename T>
    DelayedTask(TimePoint const & when, T && task) : m_when(when), m_task(std::forward<T>(task))
    {
    }

    bool operator<(DelayedTask const & rhs) const { return m_when < rhs.m_when; }
    bool operator>(DelayedTask const & rhs) const { return rhs < *this; }

    TimePoint m_when = {};
    Task m_task = {};
  };

  using ImmediateQueue = std::queue<Task>;
  using DelayedQueue =
  std::priority_queue<DelayedTask, std::vector<DelayedTask>, std::greater<DelayedTask>>;

  template <typename Fn>
  bool TouchQueues(Fn && fn)
  {
    std::lock_guard<std::mutex> lk(m_mu);
    if (m_shutdown)
      return false;
    fn();
    m_cv.notify_one();
    return true;
  }

  void ProcessTasks();

  std::vector<::threads::SimpleThread> m_threads;
  std::mutex m_mu;
  std::condition_variable m_cv;

  bool m_shutdown = false;
  Exit m_exit = Exit::SkipPending;

  ImmediateQueue m_immediate;
  DelayedQueue m_delayed;

  ThreadChecker m_checker;
};
}  // namespace delayed
}  // namespace thread_pool
}  // namespace base
