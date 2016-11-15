#pragma once

#include "std/string.hpp"

namespace routing
{
/// \brief Builds section with road restrictions.
/// \param mwmPath path to mwm which will be added with road restriction section.
/// \param restrictionPath comma separated (csv like) file with road restrictions in osm ids terms
/// in the following format:
/// <type of restrictions>, <osm id 1 of the restriction>, <osm id 2>, and so on
/// For example:
/// Only, 335049632, 49356687,
/// No, 157616940, 157616940,
/// \param osmIdsToFeatureIdsPath a binary file with mapping form osm ids to feature ids.
/// One osm id is mapped to one feature is. The file should be saved with the help of
/// OsmID2FeatureID class or using a similar way.
bool BuildRoadRestrictions(string const & mwmPath, string const & restrictionPath,
                           string const & osmIdsToFeatureIdsPath);
}  // namespace routing
