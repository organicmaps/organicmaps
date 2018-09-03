#pragma once

#include "routing/vehicle_mask.hpp"

#include <string>

namespace track_generator_tool
{
void GenerateTracks(std::string const & inputDir, std::string const & outputDir, routing::VehicleType type);
}  // namespace track_generator_tool
