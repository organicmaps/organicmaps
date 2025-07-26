#pragma once

#include <functional>

namespace threads
{
class IRoutine;
class Thread;
}  // namespace threads

namespace base
{

class ThreadPool
{
public:
  typedef std::function<void(threads::IRoutine *)> TFinishRoutineFn;

  ThreadPool(size_t size, TFinishRoutineFn const & finishFn);
  ~ThreadPool();

  // ThreadPool will not delete routine. You can delete it in TFinishRoutineFn.
  void PushBack(threads::IRoutine * routine);
  void PushFront(threads::IRoutine * routine);

  // - calls Cancel for the current processing routines
  // - joins threads
  // - calls Cancel for the remains routines in queue
  void Stop();

private:
  class Impl;
  Impl * m_impl;
};

}  // namespace base
