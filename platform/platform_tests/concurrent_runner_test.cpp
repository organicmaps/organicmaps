#include "../../testing/testing.hpp"

#include "../concurrent_runner.hpp"
#include "../platform.hpp"

#include "../../base/logging.hpp"
#include "../../base/mutex.hpp"

#include "../../std/bind.hpp"

namespace
{

class ConcurrentRunnerForTest : public threads::ConcurrentRunner
{
public:
  using threads::ConcurrentRunner::Join;
};

}

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
  ConcurrentRunnerForTest r;
  for (int i = 0; i < MAX_THREADS; ++i)
    r.Run(&f);
  r.Join();
  TEST_EQUAL(globalCounter, MAX_THREADS, ());
}
