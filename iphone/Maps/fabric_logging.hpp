#pragma once

#include "base/logging.hpp"

namespace platform
{
  void LogMessageFabric(my::LogLevel level, my::SrcPoint const & srcPoint, string const & msg);
}
