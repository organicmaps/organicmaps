#pragma once

#include "std/chrono.hpp"
#include "std/condition_variable.hpp"
#include "std/function.hpp"
#include "std/mutex.hpp"
#include "std/thread.hpp"

namespace my
{
class DeferredTask
{
  using TDuration = duration<double>;
  thread m_thread;
  mutex m_mutex;
  condition_variable m_cv;
  function<void()> m_fn;
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
      unique_lock<mutex> l(m_mutex);
      m_fn = fn;
    }
    m_cv.notify_one();
  }
};
}  // namespace my
