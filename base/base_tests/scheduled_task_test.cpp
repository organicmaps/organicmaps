#include "../../testing/testing.hpp"

#include "../scheduled_task.hpp"

#include "../../std/atomic.hpp"
#include "../../std/bind.hpp"


namespace
{
void add_int(atomic<int> & val, int a) { val += a; }

void mul_int(atomic<int> & val, int b)
{
  int value = val;
  while (!val.compare_exchange_weak(value, value * b))
    ;
}
}  // namespace

/// @todo Next tests are based on assumptions that some delays are suitable for
/// performing needed checks, before a task will fire.

UNIT_TEST(ScheduledTask_Smoke)
{
  atomic<int> val(0);

  ScheduledTask t(bind(&add_int, ref(val), 10), 1000);

  // Assume that t thread isn't fired yet.
  TEST_EQUAL(val, 0, ());

  threads::Sleep(1100);

  TEST_EQUAL(val, 10, ());
}

UNIT_TEST(ScheduledTask_CancelInfinite)
{
  atomic<int> val(2);

  ScheduledTask t0(bind(&add_int, ref(val), 10), static_cast<unsigned>(-1));

  t0.CancelBlocking();

  TEST_EQUAL(val, 2, ());
}

UNIT_TEST(ScheduledTask_Cancel)
{
  atomic<int> val(2);

  ScheduledTask t0(bind(&add_int, ref(val), 10), 500);
  ScheduledTask t1(bind(&mul_int, ref(val), 2), 1000);

  TEST_EQUAL(val, 2, ());

  // Assume that t0 thread isn't fired yet.
  t0.CancelBlocking();

  threads::Sleep(1100);

  TEST_EQUAL(val, 4, ());
}

UNIT_TEST(ScheduledTask_NoWaitInCancel)
{
  atomic<int> val(2);

  ScheduledTask t0(bind(&add_int, ref(val), 10), 1000);
  ScheduledTask t1(bind(&mul_int, ref(val), 3), 500);

  t0.CancelBlocking();

  // Assume that t1 thread isn't fired yet.
  val += 3;

  threads::Sleep(600);

  TEST_EQUAL(val, 15, ());
}
