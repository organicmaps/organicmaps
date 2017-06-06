#include "base/logging.hpp"

#include "base/assert.hpp"
#include "base/macros.hpp"
#include "base/thread.hpp"
#include "base/timer.hpp"

#include "std/target_os.hpp"

#include <algorithm>
#include <cassert>
#include <iomanip>
#include <iostream>
#include <iterator>
#include <map>
#include <mutex>
#include <sstream>
#include <vector>

using namespace std;

namespace
{
mutex g_logMutex;
}  // namespace

namespace my
{
string ToString(LogLevel level)
{
  auto const & names = GetLogLevelNames();
  CHECK_LESS(level, names.size(), ());
  return names[level];
}

bool FromString(string const & s, LogLevel & level)
{
  auto const & names = GetLogLevelNames();
  auto it = find(names.begin(), names.end(), s);
  if (it == names.end())
    return false;
  level = static_cast<LogLevel>(distance(names.begin(), it));
  return true;
}

vector<string> const & GetLogLevelNames()
{
  // If you're going to modify the behavior of the function, please,
  // check validity of LogHelper ctor.
  static vector<string> const kNames = {{"DEBUG", "INFO", "WARNING", "ERROR", "CRITICAL"}};
  return kNames;
}

class LogHelper
{
  int m_threadsCount;
  map<threads::ThreadID, int> m_threadID;

  int GetThreadID()
  {
    int & id = m_threadID[threads::GetCurrentThreadID()];
    if (id == 0)
      id = ++m_threadsCount;
    return id;
  }

  my::Timer m_timer;

  char const * m_names[5];
  size_t m_lens[5];

public:
  LogHelper() : m_threadsCount(0)
  {
    // This code highly depends on the fact that GetLogLevelNames()
    // always returns the same constant vector of strings.
    auto const & names = GetLogLevelNames();

    assert(names.size() == 5);
    for (size_t i = 0; i < 5; ++i)
    {
      m_names[i] = names[i].c_str();
      m_lens[i] = names[i].size();
    }
  }

  void WriteProlog(ostream & s, LogLevel level)
  {
    s << "LOG";

    s << " TID(" << GetThreadID() << ")";
    s << " " << m_names[level];

    double const sec = m_timer.ElapsedSeconds();
    s << " " << setfill(' ') << setw(static_cast<int>(16 - m_lens[level])) << sec << " ";
  }
};

void LogMessageDefault(LogLevel level, SrcPoint const & srcPoint, string const & msg)
{
  lock_guard<mutex> lock(g_logMutex);

  static LogHelper logger;

  ostringstream out;
  logger.WriteProlog(out, level);

  out << DebugPrint(srcPoint) << msg << endl;
  cerr << out.str();

  CHECK_LESS(level, g_LogAbortLevel, ("Abort. Log level is too serious", level));
}

void LogMessageTests(LogLevel level, SrcPoint const &, string const & msg)
{
  lock_guard<mutex> lock(g_logMutex);

  ostringstream out;
  out << msg << endl;
  cerr << out.str();

  CHECK_LESS(level, g_LogAbortLevel, ("Abort. Log level is too serious", level));
}

LogMessageFn LogMessage = &LogMessageDefault;

LogMessageFn SetLogMessageFn(LogMessageFn fn)
{
  swap(LogMessage, fn);
  return fn;
}

LogLevel GetDefaultLogLevel()
{
#if defined(DEBUG)
  return LDEBUG;
#else
  return LINFO;
#endif  // defined(DEBUG)
}

LogLevel GetDefaultLogAbortLevel()
{
#if defined(DEBUG)
  return LERROR;
#else
  return LCRITICAL;
#endif  // defined(DEBUG)
}

AtomicLogLevel g_LogLevel = {GetDefaultLogLevel()};
AtomicLogLevel g_LogAbortLevel = {GetDefaultLogAbortLevel()};
}  // namespace my
