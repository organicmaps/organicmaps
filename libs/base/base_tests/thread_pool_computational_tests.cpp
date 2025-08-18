#include "testing/testing.hpp"

#include "base/thread.hpp"
#include "base/thread_pool_computational.hpp"

#include <atomic>
#include <future>
#include <thread>
#include <vector>

namespace
{
size_t const kTimes = 500;
}  // namespace

UNIT_TEST(ThreadPoolComputational_SomeThreads)
{
  for (size_t t = 0; t < kTimes; ++t)
  {
    size_t const threadCount = 4;
    std::atomic<size_t> counter{0};
    {
      base::ComputationalThreadPool threadPool(threadCount);
      for (size_t i = 0; i < threadCount; ++i)
      {
        threadPool.Submit([&]()
        {
          threads::Sleep(1);
          ++counter;
        });
      }
    }

    TEST_EQUAL(threadCount, counter, ());
  }
}

UNIT_TEST(ThreadPoolComputational_OneThread)
{
  for (size_t t = 0; t < kTimes; ++t)
  {
    size_t const threadCount = 1;
    std::atomic<size_t> counter{0};
    {
      base::ComputationalThreadPool threadPool(threadCount);
      for (size_t i = 0; i < threadCount; ++i)
      {
        threadPool.Submit([&]()
        {
          threads::Sleep(1);
          ++counter;
        });
      }
    }

    TEST_EQUAL(threadCount, counter, ());
  }
}

UNIT_TEST(ThreadPoolComputational_ManyThread)
{
  for (size_t t = 0; t < kTimes; ++t)
  {
    size_t threadCount = std::thread::hardware_concurrency();
    CHECK_NOT_EQUAL(threadCount, 0, ());
    threadCount *= 2;
    std::atomic<size_t> counter{0};
    {
      base::ComputationalThreadPool threadPool(threadCount);
      for (size_t i = 0; i < threadCount; ++i)
      {
        threadPool.Submit([&]()
        {
          threads::Sleep(1);
          ++counter;
        });
      }
    }

    TEST_EQUAL(threadCount, counter, ());
  }
}

UNIT_TEST(ThreadPoolComputational_ReturnValue)
{
  for (size_t t = 0; t < kTimes; ++t)
  {
    size_t const threadCount = 4;
    base::ComputationalThreadPool threadPool(threadCount);
    std::vector<std::future<size_t>> futures;
    for (size_t i = 0; i < threadCount; ++i)
    {
      auto f = threadPool.Submit([=]()
      {
        threads::Sleep(1);
        return i;
      });

      futures.push_back(std::move(f));
    }

    for (size_t i = 0; i < threadCount; ++i)
      TEST_EQUAL(futures[i].get(), i, ());
  }
}

UNIT_TEST(ThreadPoolComputational_ManyTasks)
{
  for (size_t t = 0; t < kTimes; ++t)
  {
    size_t const taskCount = 11;
    std::atomic<size_t> counter{0};
    {
      base::ComputationalThreadPool threadPool(4);
      for (size_t i = 0; i < taskCount; ++i)
      {
        threadPool.Submit([&]()
        {
          threads::Sleep(1);
          ++counter;
        });
      }
    }

    TEST_EQUAL(taskCount, counter, ());
  }
}
