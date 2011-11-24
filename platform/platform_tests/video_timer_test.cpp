#include "../../testing/testing.hpp"

#include "../video_timer.hpp"

#include "../../base/thread.hpp"
#include "../../base/logging.hpp"

#include "../../std/bind.hpp"

void incrementValue(int & i)
{
  ++i;
}

UNIT_TEST(TimerTest)
{
  int i = 0;
#ifdef OMIM_OS_MAC
  VideoTimer * videoTimer = CreatePThreadVideoTimer(bind(&incrementValue, ref(i)));
#endif

  LOG(LINFO, ("checking for approximately 60 cycles in second"));

  videoTimer->start();

  threads::Sleep(1000);

  videoTimer->pause();

  TEST((i >= 57) && (i <= 61), ("timer doesn't work, i=", i));

  videoTimer->resume();

  threads::Sleep(200);

  videoTimer->stop();

  videoTimer->start();

  threads::Sleep(200);

  videoTimer->stop();
}



