#pragma once

#include <string>

namespace generator
{
bool BuildUgcMwmSection(std::string const & srcDbFilename, std::string const & mwmFile,
                        std::string const & osmToFeatureFilename);
}  // namespace generator
