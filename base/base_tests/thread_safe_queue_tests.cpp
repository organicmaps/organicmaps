#include "testing/testing.hpp"

#include "base/thread_pool_delayed.hpp"
#include "base/thread_safe_queue.hpp"

#include <chrono>
#include <cstddef>
#include <set>
#include <thread>

using namespace base::thread_pool::delayed;

UNIT_TEST(DataWrapper_DataWrapper)
{
  base::threads::DataWrapper<size_t> dw1;
  TEST(dw1.IsEmpty(), ());

  base::threads::DataWrapper<size_t> dw2(101);
  TEST(!dw2.IsEmpty(), ());
}

UNIT_TEST(DataWrapper_Get)
{
  base::threads::DataWrapper<size_t> dw(101);
  TEST(!dw.IsEmpty(), ());
  TEST_EQUAL(dw.Get(), 101, ());
}

UNIT_TEST(ThreadSafeQueue_ThreadSafeQueue)
{
  base::threads::ThreadSafeQueue<size_t> queue;

  TEST(queue.Empty(), ());
  TEST_EQUAL(queue.Size(), 0, ());
}

UNIT_TEST(ThreadSafeQueue_Push)
{
  size_t const kSize = 100;
  base::threads::ThreadSafeQueue<size_t> queue;
  ThreadPool pool(2, ThreadPool::Exit::ExecPending);
  for (size_t i = 0; i < kSize; ++i)
  {
    pool.Push([&, i](){
      queue.Push(i);
    });
  }

  pool.ShutdownAndJoin();

  TEST_EQUAL(queue.Size(), kSize, ());
}

UNIT_TEST(ThreadSafeQueue_WaitAndPop)
{
  using namespace std::chrono_literals;
  base::threads::ThreadSafeQueue<size_t> queue;
  size_t const value = 101;
  size_t result;
  auto thread = std::thread([&]() {
    std::this_thread::sleep_for(10ms);
    queue.Push(value);
  });

  queue.WaitAndPop(result);

  TEST_EQUAL(result, value, ());

  thread.join();
}

UNIT_TEST(ThreadSafeQueue_TryPop)
{
  using namespace std::chrono_literals;
  base::threads::ThreadSafeQueue<size_t> queue;
  size_t const value = 101;
  size_t result;
  auto thread = std::thread([&]() {
    std::this_thread::sleep_for(10ms);
    queue.Push(value);
  });

  TEST(!queue.TryPop(result), ());
  thread.join();

  TEST(queue.TryPop(result), ());
  TEST_EQUAL(result, value, ());
}

UNIT_TEST(ThreadSafeQueue_ExampleWithDataWrapper)
{
  size_t const kSize = 100000;
  base::threads::ThreadSafeQueue<base::threads::DataWrapper<size_t>> queue;

  auto thread = std::thread([&]() {
    while (true)
    {
      base::threads::DataWrapper<size_t> dw;
      queue.WaitAndPop(dw);
      if (dw.IsEmpty())
      {
        return;
      }
      else
      {
        ASSERT_GREATER_OR_EQUAL(dw.Get(), 0, ());
        ASSERT_LESS_OR_EQUAL(dw.Get(), kSize, ());
      }
    }
  });

  ThreadPool pool(4, ThreadPool::Exit::ExecPending);
  for (size_t i = 0; i < kSize; ++i)
  {
    pool.Push([&, i](){
      queue.Push(i);
    });
  }
  pool.Push([&](){
    queue.Push({});
  });

  thread.join();
}
