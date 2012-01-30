#include "logging.hpp"

#include <android/log.h>

#include "../../../../../base/assert.hpp"

namespace jni {

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
  __android_log_print(pr, "MapsWithMe_JNI", out.c_str());
}

void AndroidAssertMessage(SrcPoint const & src, string const & s)
{
  AndroidLogMessage(LERROR, src, s);
}

void InitSystemLog()
{
  SetLogMessageFn(&AndroidLogMessage);
}

void InitAssertLog()
{
  SetAssertFunction(&AndroidAssertMessage);
}

}
