#pragma once
#include "base/base.hpp"
#include "base/internal/message.hpp"
#include "base/src_point.hpp"

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

  typedef void (*LogMessageFn)(LogLevel level, SrcPoint const &, string const &);

  extern LogMessageFn LogMessage;
  extern LogLevel g_LogLevel;

  /// @return Pointer to previous message function.
  LogMessageFn SetLogMessageFn(LogMessageFn fn);
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
