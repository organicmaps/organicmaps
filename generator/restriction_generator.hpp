#pragma once

#include "std/string.hpp"

namespace routing
{
  bool BuildRoadRestrictions(string const & mwmPath, string const & restrictionPath,
                             string const & featureId2OsmIdsPath);
}  // namespace routing
