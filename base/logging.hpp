#pragma once

#include "base/base.hpp"
#include "base/internal/message.hpp"
#include "base/src_point.hpp"

#include <array>
#include <atomic>
#include <string>

namespace my
{
enum LogLevel
{
  LDEBUG,
  LINFO,
  LWARNING,
  LERROR,
  LCRITICAL,

  NUM_LOG_LEVELS
};

std::string ToString(LogLevel level);
bool FromString(std::string const & s, LogLevel & level);
std::array<char const *, NUM_LOG_LEVELS> const & GetLogLevelNames();

using AtomicLogLevel = std::atomic<LogLevel>;
using LogMessageFn = void (*)(LogLevel level, SrcPoint const &, std::string const &);

LogLevel GetDefaultLogLevel();
LogLevel GetDefaultLogAbortLevel();

extern LogMessageFn LogMessage;
extern AtomicLogLevel g_LogLevel;
extern AtomicLogLevel g_LogAbortLevel;

/// @return Pointer to previous message function.
LogMessageFn SetLogMessageFn(LogMessageFn fn);

void LogMessageDefault(LogLevel level, SrcPoint const & srcPoint, std::string const & msg);
void LogMessageTests(LogLevel level, SrcPoint const & srcPoint, std::string const & msg);

/// Scope Guard to temporarily suppress specific log level, for example, in unit tests:
/// ...
/// {
///   LogLevelSuppressor onlyLERRORAndLCriticalLogsAreEnabled;
///   TEST(SomeFunctionWhichHasDebugOrInfoOrWarningLogs(), ());
/// }
struct ScopedLogLevelChanger
{
  LogLevel m_old = g_LogLevel;
  ScopedLogLevelChanger(LogLevel temporaryLogLevel = LERROR) { g_LogLevel = temporaryLogLevel; }
  ~ScopedLogLevelChanger() { g_LogLevel = m_old; }
};

struct ScopedLogAbortLevelChanger
{
  LogLevel m_old = g_LogAbortLevel;
  ScopedLogAbortLevelChanger(LogLevel temporaryLogAbortLevel = LCRITICAL)
  {
    g_LogAbortLevel = temporaryLogAbortLevel;
  }
  ~ScopedLogAbortLevelChanger() { g_LogAbortLevel = m_old; }
};
}  // namespace my

using ::my::LDEBUG;
using ::my::LINFO;
using ::my::LWARNING;
using ::my::LERROR;
using ::my::LCRITICAL;
using ::my::NUM_LOG_LEVELS;

// Logging macro.
// Example usage: LOG(LINFO, (Calc(), m_Var, "Some string constant"));
#define LOG(level, msg)                                        \
  do                                                           \
  {                                                            \
    if ((level) >= ::my::g_LogLevel)                           \
      ::my::LogMessage(level, SRC(), ::my::impl::Message msg); \
  } while (false)

// Logging macro with short info (without entry point)
#define LOG_SHORT(level, msg)                                           \
  do                                                                    \
  {                                                                     \
    if ((level) >= ::my::g_LogLevel)                                    \
      ::my::LogMessage(level, my::SrcPoint(), ::my::impl::Message msg); \
  } while (false)

// Conditional log. Logs @msg with level @level in case when @X returns false.
#define CLOG(level, X, msg)                                         \
  do                                                                \
  {                                                                 \
    if (!(X))                                                         \
      LOG(level, (SRC(), "CLOG(" #X ")", ::my::impl::Message msg)); \
  } while (false)
