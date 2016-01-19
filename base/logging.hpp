#pragma once

#include "base/base.hpp"
#include "base/internal/message.hpp"
#include "base/src_point.hpp"

#include "std/atomic.hpp"

namespace my
{
  enum LogLevel
  {
    LDEBUG,
    LINFO,
    LWARNING,
    LERROR,
    LCRITICAL
  };

  typedef atomic<LogLevel> TLogLevel;
  typedef void (*LogMessageFn)(LogLevel level, SrcPoint const &, string const &);

  extern LogMessageFn LogMessage;
  extern TLogLevel g_LogLevel;
  extern TLogLevel g_LogAbortLevel;

  /// @return Pointer to previous message function.
  LogMessageFn SetLogMessageFn(LogMessageFn fn);

  void LogMessageDefault(LogLevel level, SrcPoint const & srcPoint, string const & msg);
  void LogMessageTests(LogLevel level, SrcPoint const & srcPoint, string const & msg);

  /// Scope Guard to temporarily suppress specific log level, for example, in unit tests:
  /// ...
  /// {
  ///   LogLevelSuppressor onlyLERRORAndLCriticalLogsAreEnabled;
  ///   TEST(SomeFunctionWhichHasDebugOrInfoOrWarningLogs(), ());
  /// }
  struct LogLevelSuppressor
  {
    LogLevel m_old = g_LogLevel;
    LogLevelSuppressor(LogLevel temporaryLogLevel = LERROR) { g_LogLevel = temporaryLogLevel; }
    ~LogLevelSuppressor() { g_LogLevel = m_old; }
  };
}

using ::my::LDEBUG;
using ::my::LINFO;
using ::my::LWARNING;
using ::my::LERROR;
using ::my::LCRITICAL;

// Logging macro.
// Example usage: LOG(LINFO, (Calc(), m_Var, "Some string constant"));
#define LOG(level, msg) do { if ((level) < ::my::g_LogLevel) {} \
  else { ::my::LogMessage(level, SRC(), ::my::impl::Message msg);} } while (false)

// Logging macro with short info (without entry point)
#define LOG_SHORT(level, msg) do { if ((level) < ::my::g_LogLevel) {} \
  else { ::my::LogMessage(level, my::SrcPoint(), ::my::impl::Message msg);} } while (false)
