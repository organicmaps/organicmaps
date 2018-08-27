#pragma once

#include "generator/cities_boundaries_builder.hpp"

#include <cstdint>
#include <string>
#include <vector>

namespace routing
{
/// \brief Write |cityRoadFeatureIds| to city_roads section to mwm with |dataPath|.
/// \param cityRoadFeatureIds a vector of road feature ids in cities.
void SerializeCityRoads(std::string const & dataPath, std::vector<uint64_t> && cityRoadFeatureIds);

/// \brief Builds city_roads in mwm with |dataPath|. This section contains road features
/// which have at least one point located in at least of on CityBoundary in |table|
/// \note Before a call of this method, geometry index section should be ready in mwm |dataPath|.
bool BuildCityRoads(std::string const & dataPath, generator::OsmIdToBoundariesTable & table);
}  // namespace routing
