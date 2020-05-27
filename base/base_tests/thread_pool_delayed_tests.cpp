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
  TEST(!thread.Push([&value]() { ++value; }).empty(), ());
  TEST(!thread.Push([&value]() { value *= 2; }).empty(), ());
  TEST(!thread.Push([&value]() { value = value * value * value; }).empty(), ());
  TEST(!thread.Push([&]() {
    lock_guard<mutex> lk(mu);
    done = true;
    cv.notify_one();
  }).empty(), ());

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
    TEST(!thread.Push([&value]() { ++value; }).empty(), ());
    TEST(!thread.Push([&value]() {
      for (int i = 0; i < 10; ++i)
        value *= 2;
    }).empty(), ());
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
  auto const id = thread.Push([&f, &thread]() {
    f.get();
    auto const id = thread.Push([]() { TEST(false, ("This task should not be executed")); });
    TEST(id.empty(), ());
  });
  TEST(!id.empty(), ());
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
      auto const id = thread.PushDelayed(entry.m_delay, [&]() { entry.m_end = thread.Now(); });
      TEST(!id.empty(), ());
    }

    for (int i = 0; i < kNumTasks; ++i)
    {
      auto & entry = immediateEntries[i];
      auto const id = thread.Push([&]() { entry = thread.Now(); });
      TEST(!id.empty(), ());
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

UNIT_TEST(ThreadPoolDelayed_CancelImmediate)
{
  int value = 0;

  {
    thread_pool::delayed::ThreadPool::TaskId cancelTaskId;
    thread_pool::delayed::ThreadPool thread;
    {
      auto const id = thread.Push([&value]() {
        ++value;
        testing::Wait();
      });
      TEST_EQUAL(id, "2", ());
    }

    {
      cancelTaskId = thread.Push([&]() { value += 1023; });

      TEST_EQUAL(cancelTaskId, "3", ());
    }

    {
      auto const id = thread.Push([&]() { ++value; });

      TEST_EQUAL(id, "4", ());
    }

    TEST(thread.Cancel(cancelTaskId), ());

    testing::Notify();

    thread.Shutdown(thread_pool::delayed::ThreadPool::Exit::ExecPending);
  }

  TEST_EQUAL(value, 2, ());
}

UNIT_TEST(ThreadPoolDelayed_CancelDelayed)
{
  int value = 0;

  {
    thread_pool::delayed::ThreadPool::TaskId cancelTaskId;
    thread_pool::delayed::ThreadPool thread;
    {
      auto const id = thread.Push([]() { testing::Wait(); });
      TEST_EQUAL(id, "2", ());
    }

    {
      auto const delayedId = thread.PushDelayed(milliseconds(1), [&value]() { ++value; });
      TEST_EQUAL(delayedId, "2", ());
    }

    {
      cancelTaskId = thread.PushDelayed(milliseconds(2), [&]() { value += 1023; });
      TEST_EQUAL(cancelTaskId, "3", ());
    }

    {
      auto const delayedId = thread.PushDelayed(milliseconds(3), [&value]() { ++value; });
      TEST_EQUAL(delayedId, "4", ());
    }

    TEST(thread.CancelDelayed(cancelTaskId), ());

    testing::Notify();

    thread.Shutdown(thread_pool::delayed::ThreadPool::Exit::ExecPending);
  }

  TEST_EQUAL(value, 2, ());
}
}  // namespace
