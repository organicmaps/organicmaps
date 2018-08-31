#include "base/thread_pool.hpp"

#include "base/thread.hpp"
#include "base/threaded_list.hpp"

#include <functional>
#include <memory>
#include <utility>
#include <vector>

namespace threads
{
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
  }

  class ThreadPool::Impl
  {
  public:
    Impl(size_t size, const TFinishRoutineFn & finishFn) : m_finishFn(finishFn), m_threads(size)
    {
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
      m_tasks.PushBack(routine);
    }

    void PushFront(threads::IRoutine * routine)
    {
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

      m_tasks.ProcessList([this](std::list<threads::IRoutine *> & tasks)
                          {
                            FinishTasksOnStop(tasks);
                          });
      m_tasks.Clear();
    }

  private:
    void FinishTasksOnStop(std::list<threads::IRoutine *> & tasks)
    {
      typedef std::list<threads::IRoutine *>::iterator task_iter;
      for (task_iter it = tasks.begin(); it != tasks.end(); ++it)
      {
        (*it)->Cancel();
        m_finishFn(*it);
      }
    }

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
}
