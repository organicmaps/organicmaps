#include "logging.hpp"

#include <android/log.h>
#include <cassert>

#include "../../../../../base/assert.hpp"
#include "../../../../../base/logging.hpp"
#include "../../../../../base/exception.hpp"
#include "../../../../../coding/file_writer.hpp"
#include "../../../../../platform/platform.hpp"

#include "../../../../../std/scoped_ptr.hpp"

//#define MWM_LOG_TO_FILE


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
  
void AndroidLogToFile(LogLevel l, SrcPoint const & src, string const & s)
{
  static scoped_ptr<FileWriter> file;
  
  if (file == NULL)
  {
    if (GetPlatform().WritableDir().empty())
      return;
    
    file.reset(new FileWriter(GetPlatform().WritablePathForFile("logging.txt")));
  }
  
  string srcString = DebugPrint(src) + " " + s + "\n";
  
  file->Write(srcString.c_str(), srcString.size());
  file->Flush();
}

void AndroidAssertMessage(SrcPoint const & src, string const & s)
{
#if defined(MWM_LOG_TO_FILE)
  AndroidLogToFile(LERROR, src, s);
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
  SetLogMessageFn(&AndroidLogToFile);
#else
  SetLogMessageFn(&AndroidLogMessage);
#endif
}

void InitAssertLog()
{
  SetAssertFunction(&AndroidAssertMessage);
}

}
