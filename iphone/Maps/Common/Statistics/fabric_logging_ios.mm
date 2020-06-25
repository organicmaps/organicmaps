#import <FirebaseCrashlytics/FirebaseCrashlytics.h>

#include "fabric_logging.hpp"

#include "base/assert.hpp"

#include <iostream>

namespace platform
{
void LogMessageFabric(base::LogLevel level, base::SrcPoint const & srcPoint, std::string const & msg)
{
  std::string recordType;
  switch (level)
  {
  case LINFO: recordType.assign("INFO "); break;
  case LDEBUG: recordType.assign("DEBUG "); break;
  case LWARNING: recordType.assign("WARN "); break;
  case LERROR: recordType.assign("ERROR "); break;
  case LCRITICAL: recordType.assign("FATAL "); break;
  case NUM_LOG_LEVELS: CHECK(false, ()); break;
  }

  std::string const srcString = recordType + DebugPrint(srcPoint) + " " + msg + "\n";

  [[FIRCrashlytics crashlytics] logWithFormat:@"%@", @(srcString.c_str())];
}

void IosLogMessage(base::LogLevel level, base::SrcPoint const & srcPoint, std::string const & msg)
{
  LogMessageFabric(level, srcPoint, msg);
  base::LogMessageDefault(level, srcPoint, msg);
}

bool IosAssertMessage(base::SrcPoint const & srcPoint, std::string const & msg)
{
  LogMessageFabric(base::LCRITICAL, srcPoint, msg);
  std::cerr << "ASSERT FAILED" << std::endl
            << srcPoint.FileName() << ":" << srcPoint.Line() << std::endl
            << msg << std::endl;
  return true;
}
}  //  namespace platform
