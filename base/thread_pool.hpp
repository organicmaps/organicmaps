#pragma once

#include "base/base.hpp"

#include <functional>

namespace threads
{
  class IRoutine;

  typedef std::function<void(threads::IRoutine *)> TFinishRoutineFn;

  class ThreadPool
  {
  public:
    ThreadPool(size_t size, const TFinishRoutineFn & finishFn);
    ~ThreadPool();

    // ThreadPool will not delete routine. You can delete it in finish_routine_fn if need
    void PushBack(threads::IRoutine * routine);
    void PushFront(threads::IRoutine * routine);
    void Stop();

  private:
    class Impl;
    Impl * m_impl;
  };
}
