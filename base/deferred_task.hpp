#pragma once

#include "base/thread.hpp"

#include <chrono>
#include <condition_variable>
#include <functional>
#include <mutex>

namespace my
{
class DeferredTask
{
  using TDuration = std::chrono::duration<double>;
  threads::SimpleThread m_thread;
  std::mutex m_mutex;
  std::condition_variable m_cv;
  std::function<void()> m_fn;
  TDuration m_duration;
  bool m_terminate = false;

public:
  DeferredTask(TDuration const & duration);
  ~DeferredTask();

  void Drop();

  template <typename TFn>
  void RestartWith(TFn const && fn)
  {
    {
      std::unique_lock<std::mutex> l(m_mutex);
      m_fn = fn;
    }
    m_cv.notify_one();
  }
};
}  // namespace my
