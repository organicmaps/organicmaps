#include "../scheduled_task.hpp"
#include "../thread.hpp"
#include "../../testing/testing.hpp"
#include "../../std/bind.hpp"

namespace
{
  void add_int(int & val, int a)
  {
    val += a;
  }

  void mul_int(int & val, int b)
  {
    val *= b;
  }
}


UNIT_TEST(ScheduledTask_Smoke)
{
  int val = 0;

  ScheduledTask t(bind(&add_int, ref(val), 10), 1000);

  TEST_EQUAL(val, 0, ());

  threads::Sleep(1100);

  TEST_EQUAL(val, 10, ());
}

UNIT_TEST(ScheduledTask_CancelInfinite)
{
  int val = 2;

  ScheduledTask t0(bind(&add_int, ref(val), 10), -1);

  t0.Cancel();
}

UNIT_TEST(ScheduledTask_Cancel)
{
  int val = 2;

  ScheduledTask t0(bind(&add_int, ref(val), 10), 500);
  ScheduledTask t1(bind(&mul_int, ref(val), 2), 1000);

  TEST_EQUAL(val, 2, ());

  t0.Cancel();

  threads::Sleep(1100);

  TEST_EQUAL(val, 4, ());
}

UNIT_TEST(ScheduledTask_NoWaitInCancel)
{
  int val = 2;

  ScheduledTask t0(bind(&add_int, ref(val), 10), 1000);
  ScheduledTask t1(bind(&mul_int, ref(val), 3), 500);

  t0.Cancel();

  val += 3;

  threads::Sleep(600);

  TEST_EQUAL(val, 15, ());
}
