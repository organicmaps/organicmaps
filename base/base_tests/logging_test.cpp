#include "../../base/SRC_FIRST.hpp"
#include "../../testing/testing.hpp"

#include "../logging.hpp"
#include "../../std/utility.hpp"
#include "../../std/vector.hpp"

namespace
{
  static vector<pair<my::LogLevel, string> > g_LoggedMessages;
  void TestLogMessage(my::LogLevel level, my::SrcPoint const &, string const & msg)
  {
    g_LoggedMessages.push_back(make_pair(level, msg));
  }

  bool g_SomeFunctionCalled;
  int SomeFunction()
  {
    g_SomeFunctionCalled = true;
    return 3;
  }

}

UNIT_TEST(LoggingSimple)
{
  my::LogLevel const logLevelSaved = my::g_LogLevel;
  my::g_LogLevel = LWARNING;
  g_LoggedMessages.clear();
  g_SomeFunctionCalled = false;
  void (*logMessageSaved)(my::LogLevel, my::SrcPoint const &, string const &) = my::LogMessage;
  my::LogMessage = &TestLogMessage;
  LOG(LINFO, ("This should not pass", SomeFunction()));
  //TEST_EQUAL(g_LoggedMessages, (vector<pair<my::LogLevel, string> >()), (::my::g_LogLevel));
  TEST(!g_SomeFunctionCalled, ());
  LOG(LWARNING, ("Test", SomeFunction()));
  vector<pair<my::LogLevel, string> > expectedLoggedMessages;
  expectedLoggedMessages.push_back(make_pair(LWARNING, string("Test 3")));
  //TEST_EQUAL(g_LoggedMessages, expectedLoggedMessages, ());
  TEST(g_SomeFunctionCalled, ());
  my::LogMessage = logMessageSaved;
  my::g_LogLevel = logLevelSaved;
}
