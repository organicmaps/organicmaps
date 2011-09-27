#include "../../testing/testing.hpp"

#include "../timer.hpp"
#include "../logging.hpp"


UNIT_TEST(Timer_Seconds)
{
  my::Timer timer;

  double t1 = timer.ElapsedSeconds();
  double s = 0.0;
  for (int i = 0; i < 10000000; ++i)
    s += i*0.01;
  double t2 = timer.ElapsedSeconds();

  TEST_NOT_EQUAL(s, 0.0, ("Fictive, to prevent loop optimization"));
  TEST_NOT_EQUAL(t1, t2, ("Timer values should not be equal"));

#ifndef DEBUG
  t1 = timer.ElapsedSeconds();
  for (int i = 0; i < 10000000; ++i) {}
  t2 = timer.ElapsedSeconds();

  TEST_EQUAL(t1, t2, ("Timer values should be equal: compiler loop optimization!"));
#endif
}

UNIT_TEST(Timer_CurrentStringTime)
{
  LOG(LINFO, (my::FormatCurrentTime()));
}
