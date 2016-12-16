#pragma once

#include "base/logging.hpp"

#include <string>

namespace platform
{
  void LogMessageFabric(my::LogLevel level, my::SrcPoint const & srcPoint, std::string const & msg);
}
