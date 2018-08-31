#include "testing/testing.hpp"

#include "base/thread.hpp"
#include "base/threaded_list.hpp"
#include "base/condition.hpp"
#include "base/logging.hpp"

#include <memory>

struct ConditionThread : public threads::IRoutine
{
  ThreadedList<int> * m_tl;

  ConditionThread(ThreadedList<int> * tl) : m_tl(tl)
  {}

  void Do()
  {
    m_tl->Front(true);
  }
};

UNIT_TEST(Condition_Test)
{
  ThreadedList<int> l;

  threads::Thread t0;
  t0.Create(std::make_unique<ConditionThread>(&l));

  threads::Thread t1;
  t1.Create(std::make_unique<ConditionThread>(&l));

  l.Cancel();
  t0.Join();
  t1.Join();
}
