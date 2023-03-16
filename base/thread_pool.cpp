#include "base/thread_pool.hpp"

#include "base/thread.hpp"
#include "base/threaded_list.hpp"

#include <functional>
#include <memory>
#include <utility>
#include <vector>

namespace base
{
namespace thread_pool
{
namespace routine
{
using namespace threads;
namespace
{
typedef std::function<threads::IRoutine *()> TPopRoutineFn;

class PoolRoutine : public IRoutine
{
public:
  PoolRoutine(const TPopRoutineFn & popFn, const TFinishRoutineFn & finishFn)
    : m_popFn(popFn)
    , m_finishFn(finishFn)
  {
  }

  virtual void Do()
  {
    while (!IsCancelled())
    {
      threads::IRoutine * task = m_popFn();
      if (task == NULL)
      {
        Cancel();
        continue;
      }

      if (!task->IsCancelled())
        task->Do();
      m_finishFn(task);
    }
  }

private:
  TPopRoutineFn m_popFn;
  TFinishRoutineFn m_finishFn;
};
} // namespace

class ThreadPool::Impl
{
public:
  Impl(size_t size, const TFinishRoutineFn & finishFn) : m_finishFn(finishFn), m_threads(size)
  {
    ASSERT_GREATER(size, 0, ());
    for (auto & thread : m_threads)
    {
      thread.reset(new threads::Thread());
      thread->Create(std::make_unique<PoolRoutine>(std::bind(&ThreadPool::Impl::PopFront, this), m_finishFn));
    }
  }

  ~Impl()
  {
    Stop();
  }

  void PushBack(threads::IRoutine * routine)
  {
    ASSERT(!IsStopped(), ());
    m_tasks.PushBack(routine);
  }

  void PushFront(threads::IRoutine * routine)
  {
    ASSERT(!IsStopped(), ());
    m_tasks.PushFront(routine);
  }

  threads::IRoutine * PopFront()
  {
    return m_tasks.Front(true);
  }

  void Stop()
  {
    m_tasks.Cancel();

    for (auto & thread : m_threads)
      thread->Cancel();
    m_threads.clear();

    m_tasks.ProcessList([this](std::list<threads::IRoutine *> const & tasks)
    {
      for (auto * t : tasks)
      {
        t->Cancel();
        m_finishFn(t);
      }
    });
    m_tasks.Clear();
  }

  bool IsStopped() const { return m_threads.empty(); }

private:
  ThreadedList<threads::IRoutine *> m_tasks;
  TFinishRoutineFn m_finishFn;

  std::vector<std::unique_ptr<threads::Thread>> m_threads;
};

ThreadPool::ThreadPool(size_t size, const TFinishRoutineFn & finishFn)
  : m_impl(new Impl(size, finishFn)) {}

ThreadPool::~ThreadPool()
{
  delete m_impl;
}

void ThreadPool::PushBack(threads::IRoutine * routine)
{
  m_impl->PushBack(routine);
}

void ThreadPool::PushFront(IRoutine * routine)
{
  m_impl->PushFront(routine);
}

void ThreadPool::Stop()
{
  m_impl->Stop();
}
}  // namespace routine

namespace routine_simple
{
ThreadPool::ThreadPool(size_t reserve) { m_pool.reserve(reserve); }

void ThreadPool::Add(std::unique_ptr<threads::IRoutine> && routine)
{
  m_pool.emplace_back(new threads::Thread());
  m_pool.back()->Create(move(routine));
}

void ThreadPool::Join()
{
  for (auto & thread : m_pool)
    thread->Join();
}

threads::IRoutine * ThreadPool::GetRoutine(size_t i) const { return m_pool[i]->GetRoutine(); }
}  // namespace routine_simple
}  // namespace thread_pool
}  // namespace base
