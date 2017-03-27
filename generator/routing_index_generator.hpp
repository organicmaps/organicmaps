#pragma once

#include <string>

namespace routing
{
bool BuildRoutingIndex(std::string const & filename, std::string const & country);
void BuildCrossMwmSection(std::string const & path, std::string const & mwmFile,
                          std::string const & country);
}  // namespace routing
