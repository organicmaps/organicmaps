#pragma once

#include "base/base.hpp"
#include "base/internal/message.hpp"
#include "base/src_point.hpp"

#include <array>
#include <atomic>
#include <string>

namespace base
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

// Scope guard to temporarily suppress a specific log level and all lower ones.
//
// For example, in unit tests:
// {
//   // Only LERROR and LCRITICAL log messages will be printed.
//   ScopedLogLevelChanger defaultChanger;
//   // Call a function that has LDEBUG/LINFO/LWARNING logs that you want to suppress.
//   TEST(func(), ());
// }
struct ScopedLogLevelChanger
{
  ScopedLogLevelChanger(LogLevel temporaryLogLevel = LERROR) { g_LogLevel = temporaryLogLevel; }

  ~ScopedLogLevelChanger() { g_LogLevel = m_old; }

  LogLevel m_old = g_LogLevel;
};

struct ScopedLogAbortLevelChanger
{
  ScopedLogAbortLevelChanger(LogLevel temporaryLogAbortLevel = LCRITICAL)
  {
    g_LogAbortLevel = temporaryLogAbortLevel;
  }

  ~ScopedLogAbortLevelChanger() { g_LogAbortLevel = m_old; }

  LogLevel m_old = g_LogAbortLevel;
};
}  // namespace base

using ::base::LDEBUG;
using ::base::LINFO;
using ::base::LWARNING;
using ::base::LERROR;
using ::base::LCRITICAL;
using ::base::NUM_LOG_LEVELS;

// Logging macro.
// Example usage: LOG(LINFO, (Calc(), m_Var, "Some string constant"));
#define LOG(level, msg)                                      \
  do                                                         \
  {                                                          \
    if ((level) >= ::base::g_LogLevel)                       \
      ::base::LogMessage(level, SRC(), ::base::Message msg); \
  } while (false)

// Logging macro with short info (without entry point)
#define LOG_SHORT(level, msg)                                           \
  do                                                                    \
  {                                                                     \
    if ((level) >= ::base::g_LogLevel)                                  \
      ::base::LogMessage(level, base::SrcPoint(), ::base::Message msg); \
  } while (false)

// Conditional log. Logs @msg with level @level in case when @X returns false.
#define CLOG(level, X, msg)                                     \
  do                                                            \
  {                                                             \
    if (!(X))                                                   \
      LOG(level, (SRC(), "CLOG(" #X ")", ::base::Message msg)); \
  } while (false)
