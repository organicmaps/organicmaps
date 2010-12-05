#pragma once
#include "base.hpp"
#include "internal/message.hpp"
#include "src_point.hpp"

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

  extern void (*LogMessage)(LogLevel level, SrcPoint const &, string const &);
  extern LogLevel g_LogLevel;
}

using ::my::LDEBUG;
using ::my::LINFO;
using ::my::LWARNING;
using ::my::LERROR;
using ::my::LCRITICAL;

// Logging macro.
// Example usage: LOG(LINFO, (Calc(), m_Var, "Some string constant"));
#define LOG(level, msg) if (level < ::my::g_LogLevel) {} \
  else { ::my::LogMessage(level, SRC(), ::my::impl::Message msg); }

// Logging macro with short info (without entry point)
#define LOG_SHORT(level, msg) if (level < ::my::g_LogLevel) {} \
  else { ::my::LogMessage(level, my::SrcPoint(), ::my::impl::Message msg); }
