#import <Crashlytics/Crashlytics.h>

#include "fabric_logging.hpp"

#include "std/mutex.hpp"

namespace
{
mutex g_logMutex;
}  //  namespace

namespace platform
{
void LogMessageFabric(my::LogLevel level, my::SrcPoint const & srcPoint, string const & msg)
{
  lock_guard<mutex> lock(g_logMutex);

  string recordType;
  switch (level)
  {
  case LINFO: recordType.assign("INFO "); break;
  case LDEBUG: recordType.assign("DEBUG "); break;
  case LWARNING: recordType.assign("WARN "); break;
  case LERROR: recordType.assign("ERROR "); break;
  case LCRITICAL: recordType.assign("FATAL "); break;
  }

  string const srcString = recordType + DebugPrint(srcPoint) + " " + msg + "\n";

  CLSLog(@"%@", @(srcString.c_str()));

  my::LogMessageDefault(level, srcPoint, msg);
}
}  //  namespace platform
