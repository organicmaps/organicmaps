#include "thread_pool.hpp"

#include "thread.hpp"
#include "threaded_list.hpp"

#include "../std/vector.hpp"
#include "../std/utility.hpp"
#include "../std/bind.hpp"

namespace threads
{
  namespace
  {
    typedef function<threads::IRoutine * ()> pop_routine_fn;
    class PoolRoutine : public IRoutine
    {
    public:
      PoolRoutine(const pop_routine_fn & popFn, const finish_routine_fn & finishFn)
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
      pop_routine_fn m_popFn;
      finish_routine_fn m_finishFn;
    };
  }

  class ThreadPool::Impl
  {
  public:
    Impl(size_t size, const finish_routine_fn & finishFn)
      : m_finishFn(finishFn)
    {
      m_threads.resize(size);
      for (size_t i = 0; i < size; ++i)
      {
        thread_info_t info = make_pair(new threads::Thread(), new PoolRoutine(bind(&ThreadPool::Impl::PopFront, this),
                                                                              m_finishFn));
        info.first->Create(info.second);
        m_threads[i] = info;
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

    threads::IRoutine * PopFront()
    {
      return m_tasks.Front(true);
    }

    void Stop()
    {
      m_tasks.Cancel();

      for (size_t i = 0; i < m_threads.size(); ++i)
        m_threads[i].second->Cancel();

      for (size_t i = 0; i < m_threads.size(); ++i)
      {
        m_threads[i].first->Cancel();
        delete m_threads[i].second;
        delete m_threads[i].first;
      }

      m_tasks.ProcessList(bind(&ThreadPool::Impl::FinishTasksOnStop, this, _1));
      m_tasks.Clear();
    }

  private:
    void FinishTasksOnStop(list<threads::IRoutine *> & tasks)
    {
      typedef list<threads::IRoutine *>::iterator task_iter;
      for (task_iter it = tasks.begin(); it != tasks.end(); ++it)
      {
        (*it)->Cancel();
        m_finishFn(*it);
      }
    }

  private:
    ThreadedList<threads::IRoutine *> m_tasks;
    finish_routine_fn m_finishFn;

    typedef pair<threads::Thread *, threads::IRoutine *> thread_info_t;
    vector<thread_info_t> m_threads;
  };

  ThreadPool::ThreadPool(size_t size, const finish_routine_fn & finishFn)
    : m_impl(new Impl(size, finishFn)) {}

  void ThreadPool::AddTask(threads::IRoutine * routine)
  {
    m_impl->PushBack(routine);
  }

  void ThreadPool::Stop()
  {
    m_impl->Stop();
  }
}
