#include "base/logging.hpp"
#include "base/assert.hpp"
#include "base/macros.hpp"
#include "base/mutex.hpp"
#include "base/thread.hpp"
#include "base/timer.hpp"

#include "std/target_os.hpp"
//#include "std/windows.hpp"

#include <algorithm>
#include <iomanip>
#include <iostream>
#include <iterator>
#include <mutex>
#include <sstream>
#include <vector>

namespace my
{
std::string ToString(LogLevel level)
{
  auto const & names = GetLogLevelNames();
  CHECK_LESS(level, names.size(), ());
  return names[level];
}

bool FromString(std::string const & s, LogLevel & level)
{
  auto const & names = GetLogLevelNames();
  auto it = std::find(names.begin(), names.end(), s);
  if (it == names.end())
    return false;
  level = static_cast<LogLevel>(std::distance(names.begin(), it));
  return true;
}

std::vector<std::string> const & GetLogLevelNames()
{
  static std::vector<std::string> const kNames = {
      {"DEBUG", "INFO", "WARNING", "ERROR", "CRITICAL"}};
  return kNames;
}

class LogHelper
{
  int m_threadsCount;
  std::map<threads::ThreadID, int> m_threadID;

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
    m_names[0] = "DEBUG";
    m_lens[0] = 5;
    m_names[1] = "INFO";
    m_lens[1] = 4;
    m_names[2] = "WARNING";
    m_lens[2] = 7;
    m_names[3] = "ERROR";
    m_lens[3] = 5;
    m_names[4] = "CRITICAL";
    m_lens[4] = 8;
  }

  void WriteProlog(std::ostream & s, LogLevel level)
  {
    s << "LOG";

    s << " TID(" << GetThreadID() << ")";
    s << " " << m_names[level];

    double const sec = m_timer.ElapsedSeconds();
    s << " " << std::setfill(' ') << std::setw(static_cast<int>(16 - m_lens[level])) << sec << " ";
  }
};

std::mutex g_logMutex;

void LogMessageDefault(LogLevel level, SrcPoint const & srcPoint, std::string const & msg)
{
  std::lock_guard<std::mutex> lock(g_logMutex);

  static LogHelper logger;

  std::ostringstream out;
  logger.WriteProlog(out, level);

  out << DebugPrint(srcPoint) << msg << std::endl;
  std::cerr << out.str();

  CHECK_LESS(level, g_LogAbortLevel, ("Abort. Log level is too serious", level));
}

void LogMessageTests(LogLevel level, SrcPoint const &, std::string const & msg)
{
  std::lock_guard<std::mutex> lock(g_logMutex);

  std::ostringstream out;
  out << msg << std::endl;
  std::cerr << out.str();

  CHECK_LESS(level, g_LogAbortLevel, ("Abort. Log level is too serious", level));
}

LogMessageFn LogMessage = &LogMessageDefault;

LogMessageFn SetLogMessageFn(LogMessageFn fn)
{
  std::swap(LogMessage, fn);
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

TLogLevel g_LogLevel = {GetDefaultLogLevel()};
TLogLevel g_LogAbortLevel = {GetDefaultLogAbortLevel()};
}  // namespace my
