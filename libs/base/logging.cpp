#include "base/logging.hpp"

#include "base/assert.hpp"
#include "base/timer.hpp"

#include <algorithm>
#include <atomic>
#include <cctype>
#include <iomanip>
#include <iostream>
#include <iterator>
#include <mutex>
#include <sstream>
#include <utility>

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

namespace LogHelper
{
int GetThreadID()
{
  static std::atomic<int> counter{0};
  // Ids are never reused, unlike the underlying OS thread ids.
  thread_local int const id = ++counter;
  return id;
}

void WriteProlog(std::ostream & s, LogLevel level)
{
  static Timer const kTimer;
  s << GetLogLevelNames()[level] << '(' << GetThreadID() << ") " << std::fixed << std::setprecision(5)
    << kTimer.ElapsedSeconds() << ' ';
}

void WriteLog(std::ostream & s, SrcPoint const & srcPoint, std::string const & msg)
{
  s << DebugPrint(srcPoint) << msg << std::endl;
}
}  // namespace LogHelper

void LogMessageDefault(LogLevel level, SrcPoint const & srcPoint, std::string const & msg)
{
  std::ostringstream out;
  LogHelper::WriteProlog(out, level);
  LogHelper::WriteLog(out, srcPoint, msg);

  {
    // The mutex serializes the output only, WriteProlog/WriteLog are thread-safe.
    std::lock_guard lock(g_logMutex);
    std::cerr << out.str();
  }

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
