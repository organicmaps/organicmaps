#pragma once

#include "base/thread.hpp"

#include <chrono>
#include <condition_variable>
#include <functional>
#include <mutex>

namespace base
{
class DeferredTask
{
public:
  using Duration = std::chrono::duration<double>;

  explicit DeferredTask(Duration const & duration);
  ~DeferredTask();

  void Drop();

  template <typename Fn>
  void RestartWith(Fn const && fn)
  {
    {
      std::unique_lock l(m_mutex);
      m_fn = fn;
    }
    m_cv.notify_one();
  }

private:
  threads::SimpleThread m_thread;
  std::mutex m_mutex;
  std::condition_variable m_cv;
  std::function<void()> m_fn;
  Duration m_duration;
  bool m_terminate = false;
};
}  // namespace base
