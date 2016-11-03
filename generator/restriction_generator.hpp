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
/// \param featureId2OsmIdsPath comma separated (csv like) file with mapping from feature id to osm
/// ids
/// in following format:
/// <feature id>, <osm id 1 corresponding feature id>, <osm id 2 corresponding feature id>, and so
/// on
/// For example:
/// 137999, 5170186,
/// 138000, 5170209,
bool BuildRoadRestrictions(string const & mwmPath, string const & restrictionPath,
                           string const & featureId2OsmIdsPath);
}  // namespace routing
