#include "generator/restriction_generator.hpp"
#include "generator/restrictions.hpp"

#include "base/logging.hpp"

namespace routing
{
bool BuildRoadRestrictions(string const & mwmPath, string const & restrictionPath,
                           string const & featureId2OsmIdsPath)
{
  LOG(LINFO, ("BuildRoadRestrictions(", mwmPath, ", ", restrictionPath, ", ", featureId2OsmIdsPath, ");"));
  RestrictionCollector restrictionCollector(restrictionPath, featureId2OsmIdsPath);
  if (!restrictionCollector.IsValid())
  {
    LOG(LWARNING, ("No valid restrictions for", mwmPath, "It's necessary to check if",
                   restrictionPath, "and", featureId2OsmIdsPath, "are available."));
    return false;
  }

  return true;
}
}  // namespace routing
