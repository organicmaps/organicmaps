#pragma once

#include "std/thread.hpp"
#include "std/chrono.hpp"
#include "std/mutex.hpp"
#include "std/condition_variable.hpp"
#include "std/function.hpp"


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
  DeferredTask(TDuration duration);
  ~DeferredTask();

  void Drop();

  template<typename TFn>
  void RestartWith(TFn const && fn)
  {
    {
      unique_lock<mutex> l(m_mutex);
      m_fn = fn;
    }
    m_cv.notify_one();
  }
};
