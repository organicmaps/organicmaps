#include "testing/testing.hpp"

#include "base/logging.hpp"
#include "base/thread.hpp"
#include "base/threaded_list.hpp"

#include <chrono>
#include <condition_variable>
#include <list>
#include <memory>
#include <mutex>

struct ThreadedListProcessor : public threads::IRoutine
{
  ThreadedList<int> & m_p;
  std::mutex & m_resMutex;
  std::condition_variable & m_resCond;
  std::list<int> & m_res;
  int m_id;

  ThreadedListProcessor(ThreadedList<int> & p, std::mutex & resMutex, std::condition_variable & resCond,
                        std::list<int> & res, int id)
    : m_p(p)
    , m_resMutex(resMutex)
    , m_resCond(resCond)
    , m_res(res)
    , m_id(id)
  {}

  virtual void Do()
  {
    while (!m_p.IsCancelled())
    {
      int res = m_p.Front(true /* doPop */);
      {
        std::lock_guard<std::mutex> resGuard(m_resMutex);
        m_res.push_back(res);
      }
      m_resCond.notify_one();
      LOG(LINFO, (m_id, " thread got ", res));
    }
    LOG(LINFO, (m_id, " thread is cancelled"));
  }
};

UNIT_TEST(ThreadedList)
{
  std::mutex resMutex;
  std::condition_variable resCond;
  std::list<int> res;

  ThreadedList<int> p;

  threads::Thread t0;
  t0.Create(std::make_unique<ThreadedListProcessor>(p, resMutex, resCond, res, 0));

  threads::Thread t1;
  t1.Create(std::make_unique<ThreadedListProcessor>(p, resMutex, resCond, res, 1));

  threads::Thread t2;
  t2.Create(std::make_unique<ThreadedListProcessor>(p, resMutex, resCond, res, 2));

  p.PushBack(0);
  p.PushBack(1);
  p.PushBack(2);

  std::unique_lock lock(resMutex);
  bool const completed = resCond.wait_for(lock, std::chrono::seconds(30), [&res]() { return res.size() == 3; });
  // Consumers can append default values after Cancel(), so copy the results first.
  auto result = res;
  lock.unlock();

  p.Cancel();

  t0.Join();
  t1.Join();
  t2.Join();

  TEST(completed, ("Timed out waiting for consumers", result));
  result.sort();
  TEST_EQUAL(result, (std::list<int>{0, 1, 2}), ());
}

UNIT_TEST(ThreadedList_Fifo)
{
  ThreadedList<int> p;
  for (int i = 0; i < 3; ++i)
    p.PushBack(i);
  for (int i = 0; i < 3; ++i)
  {
    int const value = p.Front(true /* doPop */);
    TEST_EQUAL(value, i, ());
  }
}
