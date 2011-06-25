#include "../../base/SRC_FIRST.hpp"
#include "../../testing/testing.hpp"

#include "../object_pool.hpp"
#include "../thread.hpp"
#include "../../base/logging.hpp"

namespace my
{
  void sleep(int ms)
  {
    timespec t;
    t.tv_nsec =(ms * 1000000) % 1000000000;
    t.tv_sec = (ms * 1000000) / 1000000000;
    nanosleep(&t, 0);
  }
}

struct ProcessorThread : public threads::IRoutine
{
  ObjectPool<int> * m_p;
  int m_data;
  list<int> * m_res;
  int m_id;

  ProcessorThread(ObjectPool<int> * p, list<int> * res, int id) : m_p(p), m_res(res), m_id(id)
  {}

  virtual void Do()
  {
    while (!m_p->IsCancelled())
    {
      int res = m_p->Reserve();
      m_res->push_back(res);
      LOG(LINFO, (m_id, " thread got ", res));
      my::sleep(10);
    }
    LOG(LINFO, (m_id, " thread is cancelled"));
  }
};

UNIT_TEST(ObjectPool)
{
  list<int> l;
  list<int> res;

  ObjectPool<int> p;
  p.Add(l);

  threads::Thread t0;
  t0.Create(new ProcessorThread(&p, &res, 0));

  threads::Thread t1;
  t1.Create(new ProcessorThread(&p, &res, 1));

  threads::Thread t2;
  t2.Create(new ProcessorThread(&p, &res, 2));

  p.Free(0);
  my::sleep(200);

  p.Free(1);
  my::sleep(200);

  p.Free(2);
  my::sleep(200);

  TEST_EQUAL(res.front(), 0, ());
  res.pop_front();
  TEST_EQUAL(res.front(), 1, ());
  res.pop_front();
  TEST_EQUAL(res.front(), 2, ());
  res.pop_front();

  p.Cancel();

  t0.Join();
  t1.Join();
  t2.Join();
}
