#pragma once

#include <cstdint>
#include <string>

namespace routing
{
// Declaration order matters. There are sections in mwm with the same order
// of subsections. New vehicle types should be added after existent types.
enum class VehicleType
{
  Pedestrian = 0,
  Bicycle = 1,
  Car = 2,
  Transit = 3,
  Count = 4
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
VehicleMask constexpr kTransitMask = GetVehicleMask(VehicleType::Transit);

std::string DebugPrint(VehicleType vehicleType);
std::string ToString(VehicleType vehicleType);
void FromString(std::string_view s, VehicleType & vehicleType);
std::string DebugPrint(VehicleMask vehicleMask);
}  // namespace routing
