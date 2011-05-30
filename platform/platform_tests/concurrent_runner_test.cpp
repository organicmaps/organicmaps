#include "../../testing/testing.hpp"

#include "../concurrent_runner.hpp"
#include "../platform.hpp"

#include "../../base/logging.hpp"
#include "../../base/mutex.hpp"

#include "../../std/bind.hpp"

int globalCounter = 0;

threads::Mutex m;

void f()
{
  threads::MutexGuard g(m);
  ++globalCounter;
}

static const int MAX_THREADS = 20;

UNIT_TEST(ConcurrentRunnerSmoke)
{
  threads::ConcurrentRunner r;
  for (int i = 0; i < MAX_THREADS; ++i)
    r.Run(&f);
  r.Join();
  TEST_EQUAL(globalCounter, MAX_THREADS, ());
}
