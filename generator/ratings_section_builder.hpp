#pragma once

#include <string>

namespace generator
{
bool BuildRatingsMwmSection(std::string const & ugcDbFilename, std::string const & mwmFile,
                            std::string const & osmToFeatureFilename);
}  // namespace generator
