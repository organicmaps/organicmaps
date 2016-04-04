#include "logging.hpp"

#include "base/exception.hpp"
#include "base/logging.hpp"

#include "coding/file_writer.hpp"

#include "platform/file_logging.hpp"
#include "platform/platform.hpp"

#include "../util/crashlytics.h"

#include <android/log.h>
#include <cassert>
#include <cstdlib>


extern crashlytics_context_t * g_crashlytics;

namespace jni
{

using namespace my;

void AndroidMessage(LogLevel level, SrcPoint const & src, string const & s)
{
  android_LogPriority pr = ANDROID_LOG_SILENT;

  switch (level)
  {
  case LINFO: pr = ANDROID_LOG_INFO; break;
  case LDEBUG: pr = ANDROID_LOG_DEBUG; break;
  case LWARNING: pr = ANDROID_LOG_WARN; break;
  case LERROR: pr = ANDROID_LOG_ERROR; break;
  case LCRITICAL: pr = ANDROID_LOG_FATAL; break;
  }

  string const out = DebugPrint(src) + " " + s;
  if (g_crashlytics)
    g_crashlytics->log(g_crashlytics, out.c_str());
  __android_log_write(pr, "MapsWithMe_JNI", out.c_str());
}

void AndroidLogMessage(LogLevel level, SrcPoint const & src, string const & s)
{
  AndroidMessage(level, src, s);
  CHECK_LESS(level, g_LogAbortLevel, ("Abort. Log level is too serious", level));
}

void AndroidAssertMessage(SrcPoint const & src, string const & s)
{
#ifdef MWM_LOG_TO_FILE
  LogMessageFile(LCRITICAL, src, s);
#else
  AndroidMessage(LCRITICAL, src, s);
#endif

#ifdef DEBUG
  assert(false);
#else
  std::abort();
#endif
}

void InitSystemLog()
{
#ifdef MWM_LOG_TO_FILE
  SetLogMessageFn(&LogMessageFile);
#else
  SetLogMessageFn(&AndroidLogMessage);
#endif
}

void InitAssertLog()
{
  SetAssertFunction(&AndroidAssertMessage);
}

}

extern "C" {

void DbgPrintC(char const * format, ...)
{
  va_list argptr;
  va_start(argptr, format);

  __android_log_vprint(ANDROID_LOG_INFO, "MapsWithMe_Debug", format, argptr);

  va_end(argptr);
}

}
