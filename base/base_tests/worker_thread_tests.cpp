#include "testing/testing.hpp"

#include "base/worker_thread.hpp"

#include <condition_variable>
#include <mutex>

using namespace base;
using namespace std;

namespace
{
UNIT_TEST(WorkerThread_Smoke)
{
  {
    WorkerThread thread;
  }

  {
    WorkerThread thread;
    thread.Shutdown(WorkerThread::Exit::SkipPending);
  }

  {
    WorkerThread thread;
    thread.Shutdown(WorkerThread::Exit::ExecPending);
  }
}

UNIT_TEST(WorkerThread_SimpleSync)
{
  int value = 0;

  mutex mu;
  condition_variable cv;
  bool done = false;

  WorkerThread thread;
  thread.Push([&value]() { ++value; });
  thread.Push([&value]() { value *= 2; });
  thread.Push([&value]() { value = value * value * value; });
  thread.Push([&]() {
    lock_guard<mutex> lk(mu);
    done = true;
    cv.notify_one();
  });

  {
    unique_lock<mutex> lk(mu);
    cv.wait(lk, [&done]() { return done; });
  }

  TEST_EQUAL(value, 8, ());
}

UNIT_TEST(WorkerThread_SimpleFlush)
{
  int value = 0;
  {
    WorkerThread thread;
    thread.Push([&value]() { ++value; });
    thread.Push([&value]() {
      for (int i = 0; i < 10; ++i)
        value *= 2;
    });
    thread.Shutdown(WorkerThread::Exit::ExecPending);
  }
  TEST_EQUAL(value, 1024, ());
}
}  // namespace
