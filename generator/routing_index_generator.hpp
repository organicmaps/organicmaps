#pragma once

#include "std/string.hpp"

namespace routing
{
bool BuildRoutingIndex(string const & filename, string const & country);
void BuildCrossMwmSection(string const & path, string const & mwmFile, string const & country);
}  // namespace routing
