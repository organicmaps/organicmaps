#include "testing/testing.hpp"

#include "base/logging.hpp"

#include "std/utility.hpp"
#include "std/vector.hpp"


namespace
{
  void TestLogMessage(my::LogLevel, my::SrcPoint const &, string const &)
  {
  }

  bool g_SomeFunctionCalled;
  int SomeFunction()
  {
    g_SomeFunctionCalled = true;
    return 3;
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
