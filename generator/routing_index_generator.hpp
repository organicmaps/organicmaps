#pragma once

#include <cstdint>
#include <string>

namespace routing
{
bool BuildRoutingIndex(std::string const & filename, std::string const & country);
bool BuildCrossMwmSection(std::string const & path, std::string const & mwmFile,
                          std::string const & country, std::string const & osmToFeatureFile,
                          bool disableCrossMwmProgress);
}  // namespace routing
