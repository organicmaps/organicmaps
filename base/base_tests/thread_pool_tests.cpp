#include "testing/testing.hpp"

#include "base/mutex.hpp"
#include "base/thread.hpp"
#include "base/thread_pool.hpp"
#include "base/condition.hpp"

#include <functional>
#include <vector>

namespace
{
  threads::Mutex g_mutex;
  const int TASK_COUNT = 10;
  class CanceledTask : public threads::IRoutine
  {
  public:
    CanceledTask()
    {
      Cancel();
    }

    virtual void Do()
    {
      TEST_EQUAL(true, false, ());
    }
  };

  void JoinFinishFunction(threads::IRoutine * routine,
                          int & finishCounter,
                          threads::Condition & cond)
  {
    cond.Lock();
    finishCounter++;
    cond.Unlock();

    delete routine;
    cond.Signal();
  }
}

UNIT_TEST(ThreadPool_CanceledTaskTest)
{
  int finishCounter = 0;
  threads::Condition cond;
  threads::ThreadPool pool(4, std::bind(&JoinFinishFunction, std::placeholders::_1,
                                        std::ref(finishCounter), std::ref(cond)));

  for (int i = 0; i < TASK_COUNT; ++i)
    pool.PushBack(new CanceledTask());

  pool.Stop();

  TEST_EQUAL(finishCounter, TASK_COUNT, ());
}

namespace
{
  class EmptyPoolTask : public threads::IRoutine
  {
  public:
    ~EmptyPoolTask()
    {
      TEST_EQUAL(IsCancelled(), true, ());
    }

    virtual void Do()
    {
      TEST_EQUAL(true, false, ());
    }
  };
}

UNIT_TEST(ThreadPool_StopOperationTest)
{
  int finishCounter = 0;
  threads::Condition cond;
  // in this case we have empty pool, and all tasks must be finish only on Stop method call
  threads::ThreadPool pool(0, std::bind(&JoinFinishFunction, std::placeholders::_1,
                                        std::ref(finishCounter), std::ref(cond)));

  for (int i = 0; i < TASK_COUNT; ++i)
    pool.PushBack(new EmptyPoolTask());

  pool.Stop();

  TEST_EQUAL(finishCounter, TASK_COUNT, ());
}

namespace
{
  class CancelTestTask : public threads::IRoutine
  {
  public:
    CancelTestTask(bool isWaitDoCall)
      : m_waitDoCall(isWaitDoCall)
      , m_doCalled(false)
    {
    }

    ~CancelTestTask()
    {
      TEST_EQUAL(m_waitDoCall, m_doCalled, ());
    }

    virtual void Do()
    {
      TEST_EQUAL(m_waitDoCall, true, ());
      m_doCalled = true;
      threads::Sleep(100);
    }

  private:
    bool m_waitDoCall;
    bool m_doCalled;
  };
}

UNIT_TEST(ThreadPool_ExecutionTaskTest)
{
  std::vector<threads::IRoutine *> tasks;
  for (int i = 0; i < TASK_COUNT - 1; ++i)
    tasks.push_back(new CancelTestTask(true));
  // CancelTastTask::Do method should not be called for last task
  tasks.push_back(new CancelTestTask(false));

  int finishCounter = 0;
  threads::Condition cond;
  threads::ThreadPool pool(4, std::bind(&JoinFinishFunction, std::placeholders::_1,
                                        std::ref(finishCounter), std::ref(cond)));

  for (size_t i = 0; i < tasks.size(); ++i)
    pool.PushBack(tasks[i]);

  // CancelTastTask::Do method should not be called for last task
  tasks.back()->Cancel();
  tasks.clear();

  while(true)
  {
    threads::ConditionGuard guard(cond);
    if (finishCounter == TASK_COUNT)
      break;
    guard.Wait();
  }
}

UNIT_TEST(ThreadPool_EmptyTest)
{
  int finishCouter = 0;
  threads::Condition cond;
  threads::ThreadPool pool(4, std::bind(&JoinFinishFunction, std::placeholders::_1,
                                        std::ref(finishCouter), std::ref(cond)));

  threads::Sleep(100);
  pool.Stop();
}
