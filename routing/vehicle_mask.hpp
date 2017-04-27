#pragma once

#include <cstdint>
#include <sstream>
#include <string>

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

inline std::string DebugPrint(VehicleType vehicleType)
{
  switch (vehicleType)
  {
  case VehicleType::Pedestrian: return "Pedestrian";
  case VehicleType::Bicycle: return "Bicycle";
  case VehicleType::Car: return "Car";
  case VehicleType::Count: return "Count";
  }
}

inline std::string DebugPrint(VehicleMask vehicleMask)
{
  std::ostringstream oss;
  oss << "VehicleMask [";
  bool first = true;
  for (size_t i = 0; i < static_cast<size_t>(VehicleType::Count); ++i)
  {
    auto const vt = static_cast<VehicleType>(i);
    if ((vehicleMask & GetVehicleMask(vt)) == 0)
      continue;

    if (!first)
      oss << ", ";
    first = false;

    oss << DebugPrint(vt);
  }
  oss << "]";
  return oss.str();
}
}  // namespace routing
