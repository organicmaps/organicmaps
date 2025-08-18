#include "testing/testing.hpp"

#include "base/thread_pool_delayed.hpp"
#include "base/thread_safe_queue.hpp"

#include <optional>
#include <thread>

UNIT_TEST(ThreadSafeQueue_ThreadSafeQueue)
{
  threads::ThreadSafeQueue<size_t> queue;

  TEST(queue.Empty(), ());
  TEST_EQUAL(queue.Size(), 0, ());
}

UNIT_TEST(ThreadSafeQueue_Push)
{
  size_t const kSize = 100;
  threads::ThreadSafeQueue<size_t> queue;
  base::DelayedThreadPool pool(2, base::DelayedThreadPool::Exit::ExecPending);
  for (size_t i = 0; i < kSize; ++i)
    pool.Push([&, i]() { queue.Push(i); });

  pool.ShutdownAndJoin();

  TEST_EQUAL(queue.Size(), kSize, ());
}

UNIT_TEST(ThreadSafeQueue_WaitAndPop)
{
  using namespace std::chrono_literals;
  threads::ThreadSafeQueue<size_t> queue;
  size_t const value = 101;
  size_t result;
  auto thread = std::thread([&]()
  {
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
  threads::ThreadSafeQueue<size_t> queue;
  size_t const value = 101;
  size_t result;
  auto thread = std::thread([&]()
  {
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
  threads::ThreadSafeQueue<std::optional<size_t>> queue;

  auto thread = std::thread([&]()
  {
    while (true)
    {
      std::optional<size_t> dw;
      queue.WaitAndPop(dw);
      if (!dw.has_value())
        return;

      ASSERT_LESS_OR_EQUAL(*dw, kSize, ());
    }
  });

  base::DelayedThreadPool pool(4, base::DelayedThreadPool::Exit::ExecPending);
  for (size_t i = 0; i < kSize; ++i)
    pool.Push([&, i]() { queue.Push(i); });
  pool.Push([&]() { queue.Push({}); });

  thread.join();
}
