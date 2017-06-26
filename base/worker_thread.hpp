#pragma once

#include "base/assert.hpp"
#include "base/thread.hpp"
#include "base/thread_checker.hpp"

#include <condition_variable>
#include <mutex>
#include <queue>
#include <utility>

namespace base
{
// This class represents a simple worker thread with a queue of tasks.
//
// *NOTE* This class IS thread-safe, but it must be destroyed on the
// same thread it was created.
class WorkerThread
{
public:
  enum class Exit
  {
    ExecPending,
    SkipPending
  };

  using Task = std::function<void()>;

  WorkerThread();
  ~WorkerThread();

  // Pushes task to the end of the thread's queue. Returns false when
  // the thread is shut down.
  template <typename T>
  bool Push(T && t)
  {
    std::lock_guard<std::mutex> lk(m_mu);
    if (m_shutdown)
      return false;
    m_queue.emplace(std::forward<T>(t));
    m_cv.notify_one();
    return true;
  }

  // Sends a signal to the thread to shut down. Returns false when the
  // thread was shut down previously.
  bool Shutdown(Exit e);

private:
  void ProcessTasks();

  threads::SimpleThread m_thread;
  std::mutex m_mu;
  std::condition_variable m_cv;

  bool m_shutdown = false;
  Exit m_exit = Exit::SkipPending;

  std::queue<Task> m_queue;

  ThreadChecker m_checker;
};
}  // namespace base
