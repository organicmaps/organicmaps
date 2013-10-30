#pragma once

#include "../std/function.hpp"

namespace threads
{
  class IRoutine;

  typedef function<void (threads::IRoutine *)> finish_routine_fn;

  class ThreadPool
  {
  public:
    ThreadPool(size_t size, const finish_routine_fn & finishFn);

    // ThreadPool will not delete routine. You can delete it in finish_routine_fn if need
    void AddTask(threads::IRoutine * routine);
    void Stop();

  private:
    class Impl;
    Impl * m_impl;
  };
}
