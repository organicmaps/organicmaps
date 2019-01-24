#include "testing/testing.hpp"

#include "base/thread_pool_delayed.hpp"

#include <condition_variable>
#include <chrono>
#include <future>
#include <mutex>

using namespace base;
using namespace std::chrono;
using namespace std;

namespace
{
UNIT_TEST(ThreadPoolDelayed_Smoke)
{
  {
    thread_pool::delayed::ThreadPool thread;
  }

  {
    thread_pool::delayed::ThreadPool thread;
    TEST(thread.Shutdown(thread_pool::delayed::ThreadPool::Exit::SkipPending), ());
  }

  {
    thread_pool::delayed::ThreadPool thread;
    TEST(thread.Shutdown(thread_pool::delayed::ThreadPool::Exit::ExecPending), ());
  }
}

UNIT_TEST(ThreadPoolDelayed_SimpleSync)
{
  int value = 0;

  mutex mu;
  condition_variable cv;
  bool done = false;

  thread_pool::delayed::ThreadPool thread;
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

UNIT_TEST(ThreadPoolDelayed_SimpleFlush)
{
  int value = 0;
  {
    thread_pool::delayed::ThreadPool thread;
    TEST(thread.Push([&value]() { ++value; }), ());
    TEST(thread.Push([&value]() {
      for (int i = 0; i < 10; ++i)
        value *= 2;
    }), ());
    TEST(thread.Shutdown(thread_pool::delayed::ThreadPool::Exit::ExecPending), ());
  }
  TEST_EQUAL(value, 1024, ());
}

UNIT_TEST(ThreadPoolDelayed_PushFromPendingTask)
{
  // promise - future pair is used as a socketpair here to pass a
  // signal from the main thread to the worker thread.
  promise<void> p;
  auto f = p.get_future();

  thread_pool::delayed::ThreadPool thread;
  bool const rv = thread.Push([&f, &thread]() {
    f.get();
    bool const rv = thread.Push([]() { TEST(false, ("This task should not be executed")); });
    TEST(!rv, ());
  });
  TEST(rv, ());
  thread.Shutdown(thread_pool::delayed::ThreadPool::Exit::ExecPending);
  p.set_value();
}

UNIT_TEST(ThreadPoolDelayed_DelayedAndImmediateTasks)
{
  int const kNumTasks = 100;

  struct DelayedTaskEntry
  {
    thread_pool::delayed::ThreadPool::TimePoint m_start = {};
    thread_pool::delayed::ThreadPool::Duration m_delay = {};
    thread_pool::delayed::ThreadPool::TimePoint m_end = {};
  };

  vector<DelayedTaskEntry> delayedEntries(kNumTasks);
  vector<thread_pool::delayed::ThreadPool::TimePoint> immediateEntries(kNumTasks);

  {
    thread_pool::delayed::ThreadPool thread;

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

    thread.Shutdown(thread_pool::delayed::ThreadPool::Exit::ExecPending);
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
