#pragma once

#include "std/cstdint.hpp"

namespace routing
{
using VehicleMask = int32_t;

int32_t constexpr kVehicleTypesNumber = 3;
VehicleMask constexpr kNumVehicleMasks = 1 << kVehicleTypesNumber;
VehicleMask constexpr kAllVehiclesMask = kNumVehicleMasks - 1;

VehicleMask constexpr kPedestrianMask = 1;
VehicleMask constexpr kBicycleMask = 2;
VehicleMask constexpr kCarMask = 4;
}  // namespace routing
