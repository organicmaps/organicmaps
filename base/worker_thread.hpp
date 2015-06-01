#pragma once

#include "base/macros.hpp"
#include "base/thread_checker.hpp"

#include "std/condition_variable.hpp"
#include "std/mutex.hpp"
#include "std/queue.hpp"
#include "std/shared_ptr.hpp"
#include "std/thread.hpp"

namespace my
{
/// This class wraps a sequential worker thread, that performs tasks
/// one-by-one. This class is not thread-safe, so, it should be
/// instantiated, used and destroyed on the same thread.
template <typename Task>
class WorkerThread
{
public:
  WorkerThread(int maxTasks)
      : m_maxTasks(maxTasks), m_shouldFinish(false), m_workerThread(&WorkerThread::Worker, this)
  {
  }

  ~WorkerThread() {
    ASSERT(m_threadChecker.CalledOnOriginalThread(), ());
    if (IsRunning())
      RunUntilIdleAndStop();
    CHECK(!IsRunning(), ());
  }

  /// Pushes new task into worker thread's queue. If the queue is
  /// full, current thread is blocked.
  ///
  /// \param task A callable object that will be called by worker thread.
  void Push(shared_ptr<Task> task)
  {
    ASSERT(m_threadChecker.CalledOnOriginalThread(), ());
    CHECK(IsRunning(), ());
    unique_lock<mutex> lock(m_mutex);
    m_condNotFull.wait(lock, [this]()
                       {
                         return m_tasks.size() < m_maxTasks;
                       });
    m_tasks.push(task);
    m_condNonEmpty.notify_one();
  }

  /// Runs worker thread until it'll become idle. After that,
  /// terminates worker thread.
  void RunUntilIdleAndStop()
  {
    ASSERT(m_threadChecker.CalledOnOriginalThread(), ());
    CHECK(IsRunning(), ());
    {
      lock_guard<mutex> lock(m_mutex);
      m_shouldFinish = true;
      m_condNonEmpty.notify_one();
    }
    m_workerThread.join();
  }

  /// \return True if worker thread is running, false otherwise.
  inline bool IsRunning() const {
    ASSERT(m_threadChecker.CalledOnOriginalThread(), ());
    return m_workerThread.joinable();
  }

private:
  void Worker()
  {
    shared_ptr<Task> task;
    while (true)
    {
      {
        unique_lock<mutex> lock(m_mutex);
        m_condNonEmpty.wait(lock, [this]()
                            {
                              return m_shouldFinish || !m_tasks.empty();
                            });
        if (m_shouldFinish && m_tasks.empty())
          break;
        task = m_tasks.front();
        m_tasks.pop();
        m_condNotFull.notify_one();
      }
      (*task)();
    }
  }

  /// Maximum number of tasks in the queue.
  int const m_maxTasks;
  queue<shared_ptr<Task>> m_tasks;

  /// When true, worker thread should finish all tasks in the queue
  /// and terminate.
  bool m_shouldFinish;

  mutex m_mutex;
  condition_variable m_condNotFull;
  condition_variable m_condNonEmpty;
  thread m_workerThread;
#ifdef DEBUG
  ThreadChecker m_threadChecker;
#endif
  DISALLOW_COPY_AND_MOVE(WorkerThread);
};  // class WorkerThread
}  // namespace my
