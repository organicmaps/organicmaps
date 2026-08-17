#pragma once

#include "base/assert.hpp"
#include "base/thread_pool_delayed.hpp"

#include <algorithm>
#include <atomic>
#include <condition_variable>
#include <memory>
#include <mutex>
#include <vector>

namespace dp
{
// This class MUST NOT run OpenGL-related tasks (which invoke OpenGL or contain any
// OpenGL data), use FR/BR threads for that.
class DrapeRoutine
{
public:
  class Result
  {
  public:
    void Wait()
    {
      if (m_isFinished)
        return;

      DrapeRoutine::Instance().Wait(m_isFinished);
    }

  private:
    friend class DrapeRoutine;

    Result() : m_isFinished(false) {}

    void Finish() { m_isFinished = true; }

    std::atomic<bool> m_isFinished;
  };

  using ResultPtr = std::shared_ptr<Result>;

  static void Init() { Instance(true /* reinitialize*/); }

  static void Shutdown() { Instance().FinishAll(); }

  template <typename Task>
  static ResultPtr Run(Task && t)
  {
    ResultPtr result(new Result());
    auto const pushResult = Instance().m_workerThread.Push([result, t = std::forward<Task>(t)]() mutable
    {
      t();
      result->Finish();
      Instance().Notify();
    });

    if (!pushResult.m_isSuccess)
      return {};

    return result;
  }

  template <typename Task>
  static ResultPtr RunDelayed(base::DelayedThreadPool::Duration const & duration, Task && t)
  {
    ResultPtr result(new Result());
    auto const pushResult =
        Instance().m_workerThread.PushDelayed(duration, [result, t = std::forward<Task>(t)]() mutable
    {
      t();
      result->Finish();
      Instance().Notify();
    });

    if (!pushResult.m_isSuccess)
      return {};

    return result;
  }

  // Asynchronous execution for tasks when execution order matters.
  template <typename Task>
  static ResultPtr RunSequential(Task && t)
  {
    ResultPtr result(new Result());
    auto const pushResult = Instance().m_sequentialWorkerThread.Push([result, t = std::forward<Task>(t)]() mutable
    {
      t();
      result->Finish();
      Instance().Notify();
    });

    if (!pushResult.m_isSuccess)
      return {};

    return result;
  }

private:
  static DrapeRoutine & Instance(bool reinitialize = false)
  {
    static std::unique_ptr<DrapeRoutine> instance;
    if (!instance || reinitialize)
    {
      if (instance)
        instance->FinishAll();
      instance = std::unique_ptr<DrapeRoutine>(new DrapeRoutine());
    }
    return *instance;
  }

  DrapeRoutine() : m_workerThread(2 /* threads count */) {}

  void Notify()
  {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_condition.notify_all();
  }

  void Wait(std::atomic<bool> const & isFinished)
  {
    std::unique_lock<std::mutex> lock(m_mutex);
    m_condition.wait(lock, [this, &isFinished]() { return m_finished || isFinished.load(); });
  }

  void FinishAll()
  {
    m_workerThread.ShutdownAndJoin();
    m_sequentialWorkerThread.ShutdownAndJoin();

    std::lock_guard<std::mutex> lock(m_mutex);
    m_finished = true;
    m_condition.notify_all();
  }

  bool m_finished = false;
  std::condition_variable m_condition;
  std::mutex m_mutex;
  base::DelayedThreadPool m_workerThread;
  base::DelayedThreadPool m_sequentialWorkerThread;
};

// This is a helper class, which aggregates logic of waiting for active
// tasks completion. It must be used when we provide tasks completion
// before subsystem shutting down.
template <typename TaskType>
class ActiveTasks
{
  struct ActiveTask
  {
    std::shared_ptr<TaskType> m_task;
    DrapeRoutine::ResultPtr m_result;

    ActiveTask(std::shared_ptr<TaskType> && task, DrapeRoutine::ResultPtr && result)
      : m_task(std::move(task))
      , m_result(std::move(result))
    {}
  };

public:
  ~ActiveTasks() { FinishAll(); }

  void Add(std::shared_ptr<TaskType> && task, DrapeRoutine::ResultPtr && result)
  {
    ASSERT(task != nullptr, ());
    ASSERT(result != nullptr, ());
    std::lock_guard<std::mutex> lock(m_mutex);
    m_tasks.emplace_back(std::move(task), std::move(result));
  }

  void Remove(std::shared_ptr<TaskType> const & task)
  {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_tasks.erase(
        std::remove_if(m_tasks.begin(), m_tasks.end(), [task](ActiveTask const & t) { return t.m_task == task; }),
        m_tasks.end());
  }

  void FinishAll()
  {
    // Move tasks to a temporary vector, because m_tasks
    // can be modified during 'Cancel' calls.
    std::vector<ActiveTask> tasks;
    {
      std::lock_guard<std::mutex> lock(m_mutex);
      tasks.swap(m_tasks);
    }

    // Cancel all tasks.
    for (auto & t : tasks)
      t.m_task->Cancel();

    // Wait for completion of unfinished tasks.
    for (auto & t : tasks)
      t.m_result->Wait();
  }

private:
  std::vector<ActiveTask> m_tasks;
  std::mutex m_mutex;
};
}  // namespace dp
