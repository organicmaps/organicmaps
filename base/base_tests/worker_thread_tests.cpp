#include "testing/testing.hpp"

#include "base/worker_thread.hpp"

#include <condition_variable>
#include <chrono>
#include <future>
#include <mutex>

using namespace base;
using namespace std::chrono;
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
    TEST(thread.Shutdown(WorkerThread::Exit::SkipPending), ());
  }

  {
    WorkerThread thread;
    TEST(thread.Shutdown(WorkerThread::Exit::ExecPending), ());
  }
}

UNIT_TEST(WorkerThread_SimpleSync)
{
  int value = 0;

  mutex mu;
  condition_variable cv;
  bool done = false;

  WorkerThread thread;
  TEST(thread.Push([&value]() { ++value; }), ());
  TEST(thread.Push([&value]() { value *= 2; }), ());
  TEST(thread.Push([&value]() { value = value * value * value; }), ());
  TEST(thread.Push([&]() {
    lock_guard<mutex> lk(mu);
    done = true;
    cv.notify_one();
  }), ());

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
    TEST(thread.Push([&value]() { ++value; }), ());
    TEST(thread.Push([&value]() {
      for (int i = 0; i < 10; ++i)
        value *= 2;
    }), ());
    TEST(thread.Shutdown(WorkerThread::Exit::ExecPending), ());
  }
  TEST_EQUAL(value, 1024, ());
}

UNIT_TEST(WorkerThread_PushFromPendingTask)
{
  // promise - future pair is used as a socketpair here to pass a
  // signal from the main thread to the worker thread.
  promise<void> p;
  auto f = p.get_future();

  WorkerThread thread;
  bool const rv = thread.Push([&f, &thread]() {
    f.get();
    bool const rv = thread.Push([]() { TEST(false, ("This task should not be executed")); });
    TEST(!rv, ());
  });
  TEST(rv, ());
  thread.Shutdown(WorkerThread::Exit::ExecPending);
  p.set_value();
}

UNIT_TEST(WorkerThread_DelayedAndImmediateTasks)
{
  int const kNumTasks = 100;

  struct DelayedTaskEntry
  {
    WorkerThread::TimePoint m_start = {};
    WorkerThread::Duration m_delay = {};
    WorkerThread::TimePoint m_end = {};
  };

  vector<DelayedTaskEntry> delayedEntries(kNumTasks);
  vector<WorkerThread::TimePoint> immediateEntries(kNumTasks);

  {
    WorkerThread thread;

    for (int i = kNumTasks - 1; i >= 0; --i)
    {
      auto & entry = delayedEntries[i];
      entry.m_start = thread.Now();
      entry.m_delay = milliseconds(i + 1);
      auto const rv = thread.PushDelayed(entry.m_delay, [&]() { entry.m_end = thread.Now(); });
      TEST(rv, ());
    }

    for (int i = 0; i < kNumTasks; ++i)
    {
      auto & entry = immediateEntries[i];
      auto const rv = thread.Push([&]() { entry = thread.Now(); });
      TEST(rv, ());
    }

    thread.Shutdown(WorkerThread::Exit::ExecPending);
  }

  for (int i = 0; i < kNumTasks; ++i)
  {
    auto const & entry = delayedEntries[i];
    TEST(entry.m_end >= entry.m_start + entry.m_delay, ("Failed delay for the delayed task", i));
  }

  for (int i = 1; i < kNumTasks; ++i)
  {
    TEST(immediateEntries[i] >= immediateEntries[i - 1],
         ("Failed delay for the immediate task", i));
  }
}
}  // namespace
