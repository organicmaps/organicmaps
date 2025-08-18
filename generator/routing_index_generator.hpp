#pragma once

#include "transit/experimental/transit_data.hpp"

#include <cstdint>
#include <functional>
#include <string>

namespace routing_builder
{
using CountryParentNameGetterFn = std::function<std::string(std::string const &)>;

bool BuildRoutingIndex(std::string const & filename, std::string const & country,
                       CountryParentNameGetterFn const & countryParentNameGetterFn);

/// \brief Builds CROSS_MWM_FILE_TAG section.
/// \note Before call of this method
/// * all features and feature geometry should be generated
/// * city_roads section should be generated
void BuildRoutingCrossMwmSection(std::string const & path, std::string const & mwmFile, std::string const & country,
                                 std::string const & intermediateDir,
                                 CountryParentNameGetterFn const & countryParentNameGetterFn,
                                 std::string const & osmToFeatureFile);

/// \brief Builds TRANSIT_CROSS_MWM_FILE_TAG section.
/// \note Before a call of this method TRANSIT_FILE_TAG should be built.
void BuildTransitCrossMwmSection(std::string const & path, std::string const & mwmFile, std::string const & country,
                                 CountryParentNameGetterFn const & countryParentNameGetterFn,
                                 ::transit::experimental::EdgeIdToFeatureId const & edgeIdToFeatureId,
                                 bool experimentalTransit = false);
}  // namespace routing_builder
