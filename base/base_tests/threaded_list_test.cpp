#include "../../base/SRC_FIRST.hpp"
#include "../../testing/testing.hpp"

#include "../threaded_list.hpp"
#include "../threaded_priority_queue.hpp"
#include "../thread.hpp"

#include "../../base/logging.hpp"


struct ThreadedListProcessor : public threads::IRoutine
{
  ThreadedList<int> * m_p;
  int m_data;
  list<int> * m_res;
  int m_id;

  ThreadedListProcessor(ThreadedList<int> * p, list<int> * res, int id)
    : m_p(p), m_res(res), m_id(id)
  {}

  virtual void Do()
  {
    while (!m_p->IsCancelled())
    {
      int res = m_p->Front(true);
      m_res->push_back(res);
      LOG(LINFO, (m_id, " thread got ", res));
      threads::Sleep(10);
    }
    LOG(LINFO, (m_id, " thread is cancelled"));
  }
};

struct ThreadedPriorityQueueProcessor : public threads::IRoutine
{
  ThreadedPriorityQueue<int> * m_p;
  int m_data;
  list<int> * m_res;
  int m_id;

  ThreadedPriorityQueueProcessor(ThreadedPriorityQueue<int> * p, list<int> * res, int id)
    : m_p(p), m_res(res), m_id(id)
  {}

  virtual void Do()
  {
    while (!m_p->IsCancelled())
    {
      int res = m_p->Top(true);
      m_res->push_back(res);
      LOG(LINFO, (m_id, " thread got ", res));
      threads::Sleep(10);
    }
    LOG(LINFO, (m_id, " thread is cancelled"));
  }
};


UNIT_TEST(ThreadedList)
{
  list<int> l;
  list<int> res;

  ThreadedList<int> p;

  threads::Thread t0;
  t0.Create(new ThreadedListProcessor(&p, &res, 0));

  threads::Thread t1;
  t1.Create(new ThreadedListProcessor(&p, &res, 1));

  threads::Thread t2;
  t2.Create(new ThreadedListProcessor(&p, &res, 2));

  p.PushBack(0);
  threads::Sleep(200);

  p.PushBack(1);
  threads::Sleep(200);

  p.PushBack(2);
  threads::Sleep(200);

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

UNIT_TEST(ThreadedPriorityQueue)
{
  list<int> res;

  ThreadedPriorityQueue<int> p;

  threads::Thread t0;
  t0.Create(new ThreadedPriorityQueueProcessor(&p, &res, 0));

  threads::Thread t1;
  t1.Create(new ThreadedPriorityQueueProcessor(&p, &res, 1));

  threads::Thread t2;
  t2.Create(new ThreadedPriorityQueueProcessor(&p, &res, 2));

  p.Push(0);
  threads::Sleep(200);

  p.Push(1);
  threads::Sleep(200);

  p.Push(2);
  threads::Sleep(200);

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

