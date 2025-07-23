#pragma once

#include "base/assert.hpp"
#include "base/bidirectional_map.hpp"
#include "base/linked_map.hpp"
#include "base/task_loop.hpp"
#include "base/thread.hpp"

#include <chrono>
#include <condition_variable>
#include <map>
#include <memory>
#include <unordered_map>
#include <vector>

namespace base
{
// This class represents a simple thread pool with a queue of tasks.
//
// *NOTE* This class IS NOT thread-safe, it must be destroyed on the
// same thread it was created, but Push* methods are thread-safe.
class DelayedThreadPool : public TaskLoop
{
public:
  using Clock = std::chrono::steady_clock;
  using Duration = Clock::duration;
  using TimePoint = Clock::time_point;

  // Use it outside the class for testing purposes only.
  static constexpr TaskId kImmediateMinId = 1;
  static constexpr TaskId kImmediateMaxId = std::numeric_limits<TaskId>::max() / 2;
  static constexpr TaskId kDelayedMinId = kImmediateMaxId + 1;
  static constexpr TaskId kDelayedMaxId = std::numeric_limits<TaskId>::max();

  enum class Exit
  {
    ExecPending,
    SkipPending
  };

  explicit DelayedThreadPool(size_t threadsCount = 1, Exit e = Exit::SkipPending);
  ~DelayedThreadPool() override;

  // Pushes task to the end of the thread's queue of immediate tasks.
  //
  // The task |t| is going to be executed after all immediate tasks
  // that were pushed pushed before it.
  PushResult Push(Task && t) override;
  PushResult Push(Task const & t) override;

  // Pushes task to the thread's queue of delayed tasks.
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
  PushResult PushDelayed(Duration const & delay, Task && t);
  PushResult PushDelayed(Duration const & delay, Task const & t);

  // Cancels task if it is in queue and is not running yet.
  // Returns false when thread is shut down,
  // task is not found or already running, otherwise true.
  bool Cancel(TaskId id);

  // Sends a signal to the thread to shut down. Returns false when the
  // thread was shut down previously.
  bool Shutdown(Exit e);

  // Sends a signal to the thread to shut down and waits for completion.
  void ShutdownAndJoin();
  bool IsShutDown();

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
    DelayedTask(TaskId id, TimePoint const & when, T && task) : m_id(id)
                                                              , m_when(when)
                                                              , m_task(std::forward<T>(task))
    {}

    bool operator<(DelayedTask const & rhs) const
    {
      if (m_when == rhs.m_when)
        return m_id < rhs.m_id;

      return m_when < rhs.m_when;
    }
    bool operator>(DelayedTask const & rhs) const { return rhs < *this; }

    TaskId m_id = kNoId;
    TimePoint m_when = {};
    Task m_task = {};
  };

  template <typename T>
  struct DeRef
  {
    bool operator()(T const & lhs, T const & rhs) const { return *lhs < *rhs; }
  };

  using ImmediateQueue = LinkedMap<TaskId, Task>;

  using DelayedValue = std::shared_ptr<DelayedTask>;
  class DelayedQueue
    : public BidirectionalMap<TaskId, DelayedValue, std::unordered_map, std::hash<TaskId>, std::multimap,
                              DeRef<DelayedValue>>
  {
  public:
    Value const & GetFirstValue() const
    {
      auto const & vTok = GetValuesToKeys();
      CHECK(!vTok.empty(), ());
      return vTok.begin()->first;
    }
  };

  template <typename T>
  PushResult AddImmediate(T && task);
  template <typename T>
  PushResult AddDelayed(Duration const & delay, T && task);
  template <typename Add>
  PushResult AddTask(Add && add);

  void ProcessTasks();

  std::vector<::threads::SimpleThread> m_threads;
  std::mutex m_mu;
  std::condition_variable m_cv;

  bool m_shutdown = false;
  Exit m_exit = Exit::SkipPending;

  ImmediateQueue m_immediate;
  DelayedQueue m_delayed;

  TaskId m_immediateLastId;
  TaskId m_delayedLastId;
};

}  // namespace base
