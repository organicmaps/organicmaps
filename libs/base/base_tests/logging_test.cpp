#include "testing/testing.hpp"

#include "base/logging.hpp"

#include <latch>
#include <set>
#include <thread>
#include <utility>
#include <vector>

namespace
{
void TestLogMessage(base::LogLevel, base::SrcPoint const &, std::string const &) {}

bool g_SomeFunctionCalled;
int SomeFunction()
{
  g_SomeFunctionCalled = true;
  return 3;
}

bool BoolFunction(bool result, bool & called)
{
  called = true;
  return result;
}
}  // namespace

UNIT_TEST(Logging_Level)
{
  base::LogLevel const logLevelSaved = base::g_LogLevel;
  base::g_LogLevel = LWARNING;

  g_SomeFunctionCalled = false;
  base::LogMessageFn logMessageSaved = base::SetLogMessageFn(&TestLogMessage);

  LOG(LINFO, ("This should not pass", SomeFunction()));
  TEST(!g_SomeFunctionCalled, ());

  LOG(LWARNING, ("This should pass", SomeFunction()));
  TEST(g_SomeFunctionCalled, ());

  base::SetLogMessageFn(logMessageSaved);
  base::g_LogLevel = logLevelSaved;
}

UNIT_TEST(NullMessage)
{
  char const * ptr = 0;
  LOG(LINFO, ("Null message test", ptr));
}

UNIT_TEST(Logging_ThreadIds)
{
  size_t constexpr kThreadsCount = 8;

  std::latch start{kThreadsCount};
  std::vector<int> ids(kThreadsCount, 0);
  std::vector<int> repeatedIds(kThreadsCount, 0);

  std::vector<std::thread> threads;
  threads.reserve(kThreadsCount);
  for (size_t i = 0; i < kThreadsCount; ++i)
  {
    threads.emplace_back([&, i]
    {
      // Race the very first (id-assigning) calls against each other.
      start.arrive_and_wait();
      ids[i] = base::LogHelper::GetThreadID();
      repeatedIds[i] = base::LogHelper::GetThreadID();
    });
  }

  for (auto & thread : threads)
    thread.join();

  TEST_EQUAL(ids, repeatedIds, ("A thread id should not change during the thread's life"));
  std::set<int> const uniqueIds(ids.begin(), ids.end());
  TEST_EQUAL(uniqueIds.size(), kThreadsCount, ("Each thread should get a unique id", ids));
}

UNIT_TEST(Logging_ConditionalLog)
{
  bool isCalled = false;
  CLOG(LINFO, BoolFunction(true, isCalled), ("This should not be displayed"));
  TEST(isCalled, ());

  isCalled = false;
  CLOG(LWARNING, BoolFunction(false, isCalled), ("This should be displayed"));
  TEST(isCalled, ());
}
