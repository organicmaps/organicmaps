#include "platform/platform.hpp"

#include "base/exception.hpp"
#include "base/logging.hpp"

#include "com/mapswithme/core/jni_helper.hpp"
#include "com/mapswithme/core/logging.hpp"
#include "com/mapswithme/core/ScopedEnv.hpp"
#include "com/mapswithme/util/crashlytics.h"

#include <android/log.h>
#include <cassert>
#include <cstdlib>


extern crashlytics_context_t * g_crashlytics;

namespace jni
{

using namespace my;

void AndroidMessage(LogLevel level, SrcPoint const & src, std::string const & s)
{
  android_LogPriority pr = ANDROID_LOG_SILENT;

  switch (level)
  {
    case LINFO: pr = ANDROID_LOG_INFO; break;
    case LDEBUG: pr = ANDROID_LOG_DEBUG; break;
    case LWARNING: pr = ANDROID_LOG_WARN; break;
    case LERROR: pr = ANDROID_LOG_ERROR; break;
    case LCRITICAL: pr = ANDROID_LOG_ERROR; break;
  }

  ScopedEnv env(jni::GetJVM());
  static jmethodID const logCoreMsgMethod = jni::GetStaticMethodID(env.get(), g_loggerFactoryClazz,
     "logCoreMessage", "(ILjava/lang/String;)V");

  std::string const out = DebugPrint(src) + " " + s;
  jni::TScopedLocalRef msg(env.get(), jni::ToJavaString(env.get(), out));
  env->CallStaticVoidMethod(g_loggerFactoryClazz, logCoreMsgMethod, pr, msg.get());
  if (g_crashlytics)
    g_crashlytics->log(g_crashlytics, out.c_str());
}

void AndroidLogMessage(LogLevel level, SrcPoint const & src, std::string const & s)
{
  AndroidMessage(level, src, s);
  CHECK_LESS(level, g_LogAbortLevel, ("Abort. Log level is too serious", level));
}

bool AndroidAssertMessage(SrcPoint const & src, std::string const & s)
{
  AndroidMessage(LCRITICAL, src, s);
  return true;
}

void InitSystemLog()
{
  SetLogMessageFn(&AndroidLogMessage);
}

void InitAssertLog()
{
  SetAssertFunction(&AndroidAssertMessage);
}

void ToggleDebugLogs(bool enabled)
{
  if (enabled)
    g_LogLevel = LDEBUG;
  else
    g_LogLevel = LINFO;
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
