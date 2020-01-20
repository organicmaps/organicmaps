#pragma once

#include "indexer/feature_altitude.hpp"

#include <string>

namespace topography_generator
{
using Altitude = feature::TAltitude;
Altitude constexpr kInvalidAltitude = feature::kInvalidAltitude;

std::string GetIsolinesFilePath(int lat, int lon, std::string const & dir);
std::string GetIsolinesFilePath(std::string const & countryId, std::string const & dir);
}  // namespace topography_generator
