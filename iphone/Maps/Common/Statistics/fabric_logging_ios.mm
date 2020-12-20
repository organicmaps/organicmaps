#include "fabric_logging.hpp"

#include "base/assert.hpp"

#include <iostream>

namespace platform
{
void LogMessageFabric(base::LogLevel level, base::SrcPoint const & srcPoint, std::string const & msg)
{
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
