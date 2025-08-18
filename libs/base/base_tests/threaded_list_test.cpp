#include "testing/testing.hpp"

#include "base/logging.hpp"
#include "base/thread.hpp"
#include "base/threaded_list.hpp"

#include <memory>
#include <mutex>

struct ThreadedListProcessor : public threads::IRoutine
{
  ThreadedList<int> & m_p;
  std::mutex & m_resMutex;
  std::list<int> & m_res;
  int m_id;

  ThreadedListProcessor(ThreadedList<int> & p, std::mutex & resMutex, std::list<int> & res, int id)
    : m_p(p)
    , m_resMutex(resMutex)
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
      LOG(LINFO, (m_id, " thread got ", res));
      threads::Sleep(10);
    }
    LOG(LINFO, (m_id, " thread is cancelled"));
  }
};

UNIT_TEST(ThreadedList)
{
  std::mutex resMutex;
  std::list<int> res;

  ThreadedList<int> p;

  threads::Thread t0;
  t0.Create(std::make_unique<ThreadedListProcessor>(p, resMutex, res, 0));

  threads::Thread t1;
  t1.Create(std::make_unique<ThreadedListProcessor>(p, resMutex, res, 1));

  threads::Thread t2;
  t2.Create(std::make_unique<ThreadedListProcessor>(p, resMutex, res, 2));

  p.PushBack(0);
  threads::Sleep(200);

  p.PushBack(1);
  threads::Sleep(200);

  p.PushBack(2);
  threads::Sleep(200);

  p.Cancel();

  t0.Join();
  t1.Join();
  t2.Join();

  TEST_EQUAL(res.front(), 0, ());
  res.pop_front();
  TEST_EQUAL(res.front(), 1, ());
  res.pop_front();
  TEST_EQUAL(res.front(), 2, ());
  res.pop_front();
}
