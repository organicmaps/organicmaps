#include "testing/testing.hpp"

#include "platform/video_timer.hpp"

#include "base/thread.hpp"
#include "base/logging.hpp"

#include "std/bind.hpp"


void incrementValue(int & i)
{
  ++i;
}

#ifdef OMIM_OS_MAC

UNIT_TEST(TimerTest)
{
  /*
  int i = 0;

  VideoTimer * videoTimer = CreatePThreadVideoTimer(bind(&incrementValue, ref(i)));

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
  */
}

#endif
