#include "platform/platform.hpp"

#include "base/exception.hpp"
#include "base/logging.hpp"

#include "app/organicmaps/sdk/core/ScopedEnv.hpp"
#include "app/organicmaps/sdk/core/jni_helper.hpp"
#include "app/organicmaps/sdk/core/logging.hpp"

#include <android/log.h>
#include <cassert>
#include <cstdlib>

namespace jni
{
namespace
{
void AndroidMessage(base::LogLevel level, base::SrcPoint const & src, std::string const & s)
{
  android_LogPriority pr = ANDROID_LOG_SILENT;

  switch (level)
  {
  case LINFO: pr = ANDROID_LOG_INFO; break;
  case LDEBUG: pr = ANDROID_LOG_DEBUG; break;
  case LWARNING: pr = ANDROID_LOG_WARN; break;
  case LERROR: pr = ANDROID_LOG_ERROR; break;
  case LCRITICAL: pr = ANDROID_LOG_ERROR; break;
  case NUM_LOG_LEVELS: break;
  }

  ScopedEnv env(GetJVM());
  static jmethodID const logMethod = GetStaticMethodID(env.get(), g_loggerClazz, "log",
                                                       "(ILjava/lang/String;Ljava/lang/String;Ljava/lang/Throwable;)V");

  std::string const out = DebugPrint(src) + s;
  TScopedLocalRef msg(env.get(), ToJavaString(env.get(), out));
  env->CallStaticVoidMethod(g_loggerClazz, logMethod, pr, NULL, msg.get(), NULL);
}

void AndroidLogMessage(base::LogLevel level, base::SrcPoint const & src, std::string const & s)
{
  AndroidMessage(level, src, s);
  CHECK_LESS(level, base::g_LogAbortLevel, ("Abort. Log level is too serious", level));
}

bool AndroidAssertMessage(base::SrcPoint const & src, std::string const & s)
{
  AndroidMessage(LCRITICAL, src, s);
  return true;
}
}  // namespace

void InitSystemLog()
{
  base::SetLogMessageFn(&AndroidLogMessage);
}

void InitAssertLog()
{
  base::SetAssertFunction(&AndroidAssertMessage);
}

void ToggleDebugLogs(bool enabled)
{
  base::g_LogLevel = enabled ? LDEBUG : LINFO;
}
}  // namespace jni
