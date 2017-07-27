#import <Crashlytics/Crashlytics.h>

#include "fabric_logging.hpp"

#include "base/assert.hpp"

#include <iostream>

namespace platform
{
void LogMessageFabric(my::LogLevel level, my::SrcPoint const & srcPoint, std::string const & msg)
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

  CLSLog(@"%@", @(srcString.c_str()));
}

void IosLogMessage(my::LogLevel level, my::SrcPoint const & srcPoint, std::string const & msg)
{
  LogMessageFabric(level, srcPoint, msg);
  my::LogMessageDefault(level, srcPoint, msg);
}

bool IosAssertMessage(my::SrcPoint const & srcPoint, std::string const & msg)
{
  LogMessageFabric(my::LCRITICAL, srcPoint, msg);
  std::cerr << "ASSERT FAILED" << std::endl
            << srcPoint.FileName() << ":" << srcPoint.Line() << std::endl
            << msg << std::endl;
  return true;
}
}  //  namespace platform
