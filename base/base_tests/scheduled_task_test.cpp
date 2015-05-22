#include "testing/testing.hpp"

#include "base/scheduled_task.hpp"

#include "std/atomic.hpp"
#include "std/bind.hpp"

namespace
{
milliseconds const kTimeInaccuracy(1);

void AddInt(atomic<int> & value, int a) { value += a; }

void MulInt(atomic<int> & value, int m)
{
  int v = value;
  while (!value.compare_exchange_weak(v, v * m))
    ;
}
}  // namespace

// ScheduledTask start (stop) is a memory barrier because it starts
// (stops) a thread. That's why it's ok to create time points before
// ScheduledTask creation and after ScheduledTask completion. Also,
// we're assuming that steady_clocks are consistent between CPU cores.
UNIT_TEST(ScheduledTask_SimpleAdd)
{
  steady_clock::time_point const start = steady_clock::now();
  milliseconds const delay(1000);

  atomic<int> value(0);
  ScheduledTask task1(bind(&AddInt, ref(value), 1), delay);
  ScheduledTask task2(bind(&AddInt, ref(value), 2), delay);
  task1.WaitForCompletion();
  task2.WaitForCompletion();
  TEST_EQUAL(value, 3, ());

  steady_clock::time_point const end = steady_clock::now();
  milliseconds const elapsed = duration_cast<milliseconds>(end - start);
  TEST(elapsed >= delay - kTimeInaccuracy, (elapsed.count(), delay.count()));
}

UNIT_TEST(ScheduledTask_SimpleMul)
{
  steady_clock::time_point const start = steady_clock::now();
  milliseconds const delay(1500);

  atomic<int> value(1);
  ScheduledTask task1(bind(&MulInt, ref(value), 2), delay);
  ScheduledTask task2(bind(&MulInt, ref(value), 3), delay);
  task1.WaitForCompletion();
  task2.WaitForCompletion();
  TEST_EQUAL(value, 6, ());

  steady_clock::time_point const end = steady_clock::now();
  milliseconds const elapsed = duration_cast<milliseconds>(end - start);
  TEST(elapsed >= delay - kTimeInaccuracy, (elapsed.count(), delay.count()));
}

UNIT_TEST(ScheduledTask_CancelNoBlocking)
{
  steady_clock::time_point const start = steady_clock::now();
  milliseconds const delay(1500);

  atomic<int> value(0);
  ScheduledTask task(bind(&AddInt, ref(value), 1), delay);

  task.CancelNoBlocking();
  task.WaitForCompletion();

  if (task.WasStarted())
  {
    TEST_EQUAL(value, 1, ());
    steady_clock::time_point const end = steady_clock::now();
    milliseconds const elapsed = duration_cast<milliseconds>(end - start);
    TEST(elapsed >= delay - kTimeInaccuracy, (elapsed.count(), delay.count()));
  }
}
