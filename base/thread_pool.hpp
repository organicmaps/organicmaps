#pragma once

#include "base/base.hpp"
#include "base/macros.hpp"

#include <functional>
#include <memory>

namespace threads
{
class IRoutine;
class Thread;
}  // namespace threads

namespace base
{
namespace thread_pool
{
namespace routine
{
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
}  // namespace routine

namespace routine_simple
{
/// Simple threads container. Takes ownership for every added IRoutine.
class ThreadPool
{
public:
  ThreadPool(size_t reserve = 0);

  void Add(std::unique_ptr<threads::IRoutine> && routine);
  void Join();

  threads::IRoutine * GetRoutine(size_t i) const;

private:
    std::vector<std::unique_ptr<threads::Thread>> m_pool;

    DISALLOW_COPY(ThreadPool);
};
}  // namespace routine_simple
}  // namespace thread_pool
}  // namespace base
