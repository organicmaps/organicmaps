#include "base/logging.hpp"

#include "base/assert.hpp"
#include "base/thread.hpp"

#include <algorithm>
#include <cassert>
#include <cstring>
#include <iomanip>
#include <iostream>
#include <iterator>
#include <mutex>
#include <sstream>

namespace base
{
namespace
{
std::mutex g_logMutex;
}  // namespace

std::string ToString(LogLevel level)
{
  auto const & names = GetLogLevelNames();
  CHECK_LESS(level, names.size(), ());
  return ::DebugPrint(names[level]);
}

std::optional<LogLevel> FromString(std::string const & s)
{
  ASSERT(!s.empty(), ("Log level should not be empty"));

  auto const & names = GetLogLevelNames();
  auto const it = std::find(names.begin(), names.end(), std::toupper(s[0]));
  if (it == names.end())
    return {};
  return static_cast<LogLevel>(std::distance(names.begin(), it));
}

std::array<char, NUM_LOG_LEVELS> const & GetLogLevelNames()
{
  static std::array<char, NUM_LOG_LEVELS> constexpr kLogLevelNames{'D', 'I', 'W', 'E', 'C'};
  return kLogLevelNames;
}

// static
LogHelper & LogHelper::Instance()
{
  static LogHelper instance;
  return instance;
}

int LogHelper::GetThreadID()
{
  int & id = m_threadID[threads::GetCurrentThreadID()];
  if (id == 0)
    id = ++m_threadsCount;
  return id;
}

void LogHelper::WriteProlog(std::ostream & s, LogLevel level)
{
  double const sec = m_timer.ElapsedSeconds();
  s << GetLogLevelNames()[level] << '(' << GetThreadID() << ") " << std::fixed << std::setprecision(5) << sec << ' ';
}

void LogHelper::WriteLog(std::ostream & s, SrcPoint const & srcPoint, std::string const & msg)
{
  s << DebugPrint(srcPoint) << msg << std::endl;
}

void LogMessageDefault(LogLevel level, SrcPoint const & srcPoint, std::string const & msg)
{
  auto & logger = LogHelper::Instance();
  std::ostringstream out;

  std::lock_guard lock(g_logMutex);
  logger.WriteProlog(out, level);
  logger.WriteLog(out, srcPoint, msg);

  std::cerr << out.str();

  CHECK_LESS(level, g_LogAbortLevel, ("Abort. Log level is too serious", level));
}

void LogMessageTests(LogLevel level, SrcPoint const &, std::string const & msg)
{
  {
    std::lock_guard lock(g_logMutex);
    std::cerr << msg << std::endl;
  }

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

AtomicLogLevel g_LogLevel = {GetDefaultLogLevel()};
AtomicLogLevel g_LogAbortLevel = {GetDefaultLogAbortLevel()};
}  // namespace base
