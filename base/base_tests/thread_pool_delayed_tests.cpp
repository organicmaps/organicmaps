#include "testing/testing.hpp"

#include "base/thread_pool_delayed.hpp"

#include <condition_variable>
#include <chrono>
#include <future>
#include <mutex>

using namespace base;
using namespace base::thread_pool::delayed;
using namespace std::chrono;
using namespace std;

namespace
{
UNIT_TEST(ThreadPoolDelayed_Smoke)
{
  {
    ThreadPool thread;
  }

  {
    ThreadPool thread;
    TEST(thread.Shutdown(ThreadPool::Exit::SkipPending), ());
  }

  {
    ThreadPool thread;
    TEST(thread.Shutdown(ThreadPool::Exit::ExecPending), ());
  }
}

UNIT_TEST(ThreadPoolDelayed_SimpleSync)
{
  int value = 0;

  mutex mu;
  condition_variable cv;
  bool done = false;

  ThreadPool thread;
  auto pushResult = thread.Push([&value]() { ++value; });
  TEST(pushResult.m_isSuccess, ());
  TEST_NOT_EQUAL(pushResult.m_id, ThreadPool::kNoId, ());

  pushResult = thread.Push([&value]() { value *= 2; });
  TEST(pushResult.m_isSuccess, ());
  TEST_NOT_EQUAL(pushResult.m_id, ThreadPool::kNoId, ());

  pushResult = thread.Push([&value]() { value = value * value * value; });
  TEST(pushResult.m_isSuccess, ());
  TEST_NOT_EQUAL(pushResult.m_id, ThreadPool::kNoId, ());

  pushResult = thread.Push([&]() {
    lock_guard<mutex> lk(mu);
    done = true;
    cv.notify_one();
  });
  TEST(pushResult.m_isSuccess, ());
  TEST_NOT_EQUAL(pushResult.m_id, ThreadPool::kNoId, ());

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
    ThreadPool thread;
    auto pushResult = thread.Push([&value]() { ++value; });
    TEST(pushResult.m_isSuccess, ());
    TEST_NOT_EQUAL(pushResult.m_id, ThreadPool::kNoId, ());

    pushResult = thread.Push([&value]() {
      for (int i = 0; i < 10; ++i)
        value *= 2;
    });
    TEST(pushResult.m_isSuccess, ());
    TEST_NOT_EQUAL(pushResult.m_id, ThreadPool::kNoId, ());

    TEST(thread.Shutdown(ThreadPool::Exit::ExecPending), ());
  }
  TEST_EQUAL(value, 1024, ());
}

UNIT_TEST(ThreadPoolDelayed_PushFromPendingTask)
{
  // promise - future pair is used as a socketpair here to pass a
  // signal from the main thread to the worker thread.
  promise<void> p;
  auto f = p.get_future();

  ThreadPool thread;
  auto const pushResult = thread.Push([&f, &thread]() {
    f.get();
    auto const innerPushResult = thread.Push([]() { TEST(false, ("This task should not be executed")); });
    TEST(!innerPushResult.m_isSuccess, ());
    TEST_EQUAL(innerPushResult.m_id, ThreadPool::kNoId, ());
  });
  TEST(pushResult.m_isSuccess, ());
  TEST_NOT_EQUAL(pushResult.m_id, ThreadPool::kNoId, ());

  thread.Shutdown(ThreadPool::Exit::ExecPending);
  p.set_value();
}

UNIT_TEST(ThreadPoolDelayed_DelayedAndImmediateTasks)
{
  int const kNumTasks = 100;

  struct DelayedTaskEntry
  {
    ThreadPool::TimePoint m_start = {};
    ThreadPool::Duration m_delay = {};
    ThreadPool::TimePoint m_end = {};
  };

  vector<DelayedTaskEntry> delayedEntries(kNumTasks);
  vector<ThreadPool::TimePoint> immediateEntries(kNumTasks);

  {
    ThreadPool thread;

    for (int i = kNumTasks - 1; i >= 0; --i)
    {
      auto & entry = delayedEntries[i];
      entry.m_start = thread.Now();
      entry.m_delay = milliseconds(i + 1);

      auto const pushResult = thread.PushDelayed(entry.m_delay, [&]() { entry.m_end = thread.Now(); });
      TEST(pushResult.m_isSuccess, ());
      TEST_NOT_EQUAL(pushResult.m_id, ThreadPool::kNoId, ());
    }

    for (int i = 0; i < kNumTasks; ++i)
    {
      auto & entry = immediateEntries[i];
      auto const pushResult = thread.Push([&]() { entry = thread.Now(); });

      TEST(pushResult.m_isSuccess, ());
      TEST_NOT_EQUAL(pushResult.m_id, ThreadPool::kNoId, ());
    }

    thread.Shutdown(ThreadPool::Exit::ExecPending);
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
    TaskLoop::TaskId cancelTaskId;
    ThreadPool thread;
    {
      auto const pushResult = thread.Push([&value]() {
        ++value;
        testing::Wait();
      });
      TEST(pushResult.m_isSuccess, ());
      TEST_EQUAL(pushResult.m_id, ThreadPool::kImmediateMinId, ());
    }

    {
      auto const pushResult = thread.Push([&]() { value += 1023; });
      TEST(pushResult.m_isSuccess, ());
      TEST_EQUAL(pushResult.m_id, ThreadPool::kImmediateMinId + 1, ());

      cancelTaskId = pushResult.m_id;
    }

    {
      auto const pushResult = thread.Push([&]() { ++value; });
      TEST(pushResult.m_isSuccess, ());
      TEST_EQUAL(pushResult.m_id, ThreadPool::kImmediateMinId + 2, ());
    }

    TEST(thread.Cancel(cancelTaskId), ());

    testing::Notify();

    thread.Shutdown(ThreadPool::Exit::ExecPending);
  }

  TEST_EQUAL(value, 2, ());
}

UNIT_TEST(ThreadPoolDelayed_CancelDelayed)
{
  int value = 0;

  {
    TaskLoop::TaskId cancelTaskId;
    ThreadPool thread;
    {
      auto const pushResult = thread.Push([]() { testing::Wait(); });
      TEST(pushResult.m_isSuccess, ());
      TEST_EQUAL(pushResult.m_id, ThreadPool::kImmediateMinId, ());
    }

    {
      auto const pushResult = thread.PushDelayed(milliseconds(1), [&value]() { ++value; });
      TEST(pushResult.m_isSuccess, ());
      TEST_EQUAL(pushResult.m_id, ThreadPool::kDelayedMinId, ());
    }

    {
      auto const pushResult = thread.PushDelayed(milliseconds(2), [&]() { value += 1023; });
      TEST(pushResult.m_isSuccess, ());
      TEST_EQUAL(pushResult.m_id, ThreadPool::kDelayedMinId + 1, ());

      cancelTaskId = pushResult.m_id;
    }

    {
      auto const pushResult = thread.PushDelayed(milliseconds(3), [&value]() { ++value; });
      TEST(pushResult.m_isSuccess, ());
      TEST_EQUAL(pushResult.m_id, ThreadPool::kDelayedMinId + 2, ());
    }

    TEST(thread.Cancel(cancelTaskId), ());

    testing::Notify();

    thread.Shutdown(ThreadPool::Exit::ExecPending);
  }

  TEST_EQUAL(value, 2, ());
}
}  // namespace
