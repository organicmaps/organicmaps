#include "logging.hpp"

#include <android/log.h>
#include <cassert>

#include "base/exception.hpp"
#include "base/logging.hpp"

#include "coding/file_writer.hpp"

#include "platform/file_logging.hpp"
#include "platform/platform.hpp"


namespace jni
{

using namespace my;

void AndroidLogMessage(LogLevel l, SrcPoint const & src, string const & s)
{
  android_LogPriority pr = ANDROID_LOG_SILENT;

  switch (l)
  {
  case LINFO: pr = ANDROID_LOG_INFO; break;
  case LDEBUG: pr = ANDROID_LOG_DEBUG; break;
  case LWARNING: pr = ANDROID_LOG_WARN; break;
  case LERROR: pr = ANDROID_LOG_ERROR; break;
  case LCRITICAL: pr = ANDROID_LOG_FATAL; break;
  }

  string const out = DebugPrint(src) + " " + s;
  __android_log_write(pr, "MapsWithMe_JNI", out.c_str());
}

void AndroidAssertMessage(SrcPoint const & src, string const & s)
{
#if defined(MWM_LOG_TO_FILE)
  LogMessageFile(LERROR, src, s);
#else
  AndroidLogMessage(LERROR, src, s);
#endif

#ifdef DEBUG
    assert(false);
#else
    MYTHROW(RootException, (s));
#endif
}

void InitSystemLog()
{
#if defined(MWM_LOG_TO_FILE)
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
