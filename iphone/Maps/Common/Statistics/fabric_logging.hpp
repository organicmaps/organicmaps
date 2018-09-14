#pragma once

#include "base/logging.hpp"

#include <string>

namespace platform
{
  void IosLogMessage(base::LogLevel level, base::SrcPoint const & srcPoint, std::string const & msg);
  bool IosAssertMessage(base::SrcPoint const & srcPoint, std::string const & msg);
}
