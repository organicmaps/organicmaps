#pragma once

#include <string>

namespace generator
{
bool BuildPopularPlacesMwmSection(std::string const & srcFilename, std::string const & mwmFile,
                                  std::string const & osmToFeatureFilename);
}  // namespace generator
