#pragma once

#include "std/cstdint.hpp"

namespace routing
{
enum class VehicleType
{
  Pedestrian = 0,
  Bicycle = 1,
  Car = 2,
  Count = 3
};

using VehicleMask = uint32_t;

inline constexpr VehicleMask GetVehicleMask(VehicleType vehicleType)
{
  return static_cast<VehicleMask>(1) << static_cast<uint32_t>(vehicleType);
}

VehicleMask constexpr kNumVehicleMasks = GetVehicleMask(VehicleType::Count);
VehicleMask constexpr kAllVehiclesMask = kNumVehicleMasks - 1;

VehicleMask constexpr kPedestrianMask = GetVehicleMask(VehicleType::Pedestrian);
VehicleMask constexpr kBicycleMask = GetVehicleMask(VehicleType::Bicycle);
VehicleMask constexpr kCarMask = GetVehicleMask(VehicleType::Car);
}  // namespace routing
