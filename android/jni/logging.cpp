#include "logging.h"

#include <android/log.h>

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

  __android_log_print(pr, "mapswithme", s.c_str());
}

void InitSystemLog()
{
  SetLogMessageFn(&AndroidLogMessage);
}

}
