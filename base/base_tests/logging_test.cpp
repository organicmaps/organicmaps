#include "testing/testing.hpp"

#include "base/logging.hpp"

#include <utility>
#include <vector>


namespace
{
  void TestLogMessage(my::LogLevel, my::SrcPoint const &, std::string const &)
  {
  }

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
}

UNIT_TEST(Logging_Level)
{
  my::LogLevel const logLevelSaved = my::g_LogLevel;
  my::g_LogLevel = LWARNING;

  g_SomeFunctionCalled = false;
  my::LogMessageFn logMessageSaved = my::SetLogMessageFn(&TestLogMessage);

  LOG(LINFO, ("This should not pass", SomeFunction()));
  TEST(!g_SomeFunctionCalled, ());

  LOG(LWARNING, ("This should pass", SomeFunction()));
  TEST(g_SomeFunctionCalled, ());

  my::SetLogMessageFn(logMessageSaved);
  my::g_LogLevel = logLevelSaved;
}

UNIT_TEST(NullMessage)
{
  char const * ptr = 0;
  LOG(LINFO, ("Null message test", ptr));
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
