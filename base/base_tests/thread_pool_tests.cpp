#include "testing/testing.hpp"

#include "base/thread.hpp"
#include "base/thread_pool.hpp"

#include <condition_variable>
#include <functional>
#include <mutex>
#include <vector>

namespace
{
  std::mutex g_mutex;
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
  struct Condition
  {
    std::mutex m;
    std::condition_variable cv;
  };

  void JoinFinishFunction(threads::IRoutine * routine,
                          int & finishCounter,
                          Condition & cond)
  {
    cond.m.lock();
    finishCounter++;
    cond.m.unlock();

    delete routine;
    cond.cv.notify_one();
  }
}

UNIT_TEST(ThreadPool_CanceledTaskTest)
{
  int finishCounter = 0;
  Condition cond;
  base::thread_pool::routine::ThreadPool pool(4, std::bind(&JoinFinishFunction, std::placeholders::_1,
                                        std::ref(finishCounter), std::ref(cond)));

  for (int i = 0; i < TASK_COUNT; ++i)
    pool.PushBack(new CanceledTask());

  pool.Stop();

  TEST_EQUAL(finishCounter, TASK_COUNT, ());
}


namespace
{
  class CancelTestTask : public threads::IRoutine
  {
  public:
    explicit CancelTestTask(bool isWaitDoCall)
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
  Condition cond;
  base::thread_pool::routine::ThreadPool pool(4, std::bind(&JoinFinishFunction, std::placeholders::_1,
                                        std::ref(finishCounter), std::ref(cond)));

  for (size_t i = 0; i < tasks.size(); ++i)
    pool.PushBack(tasks[i]);

  // CancelTastTask::Do method should not be called for last task
  tasks.back()->Cancel();
  tasks.clear();

  while(true)
  {
    std::unique_lock lock(cond.m);
    if (finishCounter == TASK_COUNT)
      break;
    cond.cv.wait(lock);
  }
}

UNIT_TEST(ThreadPool_EmptyTest)
{
  int finishCouter = 0;
  Condition cond;
  base::thread_pool::routine::ThreadPool pool(4, std::bind(&JoinFinishFunction, std::placeholders::_1,
                                        std::ref(finishCouter), std::ref(cond)));

  threads::Sleep(100);
  pool.Stop();
}
