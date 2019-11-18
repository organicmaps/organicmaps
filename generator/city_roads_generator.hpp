#pragma once

#include "generator/cities_boundaries_builder.hpp"

#include <cstdint>
#include <string>
#include <vector>

namespace routing
{
/// \brief Write |cityRoadFeatureIds| to city_roads section to mwm with |dataPath|.
/// \param cityRoadFeatureIds a vector of road feature ids in cities.
void SerializeCityRoads(std::string const & mwmPath, std::vector<uint32_t> && cityRoadFeatureIds);

/// \brief Marks road-features as city roads if they covered some boundary from data dumped in
/// |boundariesPath|. Then serialize ids of such features to "city_roads" section.
bool BuildCityRoads(std::string const & mwmPath, std::string const & boundariesPath);
}  // namespace routing
