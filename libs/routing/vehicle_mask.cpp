#include "routing/vehicle_mask.hpp"

#include <sstream>
#include <string>

#include "base/assert.hpp"

namespace routing
{
std::string_view DebugPrint(VehicleType vehicleType)
{
  switch (vehicleType)
  {
    using enum VehicleType;
  case Pedestrian: return "Pedestrian";
  case Bicycle: return "Bicycle";
  case Car: return "Car";
  case Transit: return "Transit";
  case Count: return "Count";
  }
  UNREACHABLE();
}

std::string ToString(VehicleType vehicleType)
{
  return std::string{DebugPrint(vehicleType)};
}

void FromString(std::string_view s, VehicleType & vehicleType)
{
  if (s == "Pedestrian")
    vehicleType = VehicleType::Pedestrian;
  else if (s == "Bicycle")
    vehicleType = VehicleType::Bicycle;
  else if (s == "Car")
    vehicleType = VehicleType::Car;
  else if (s == "Transit")
    vehicleType = VehicleType::Transit;
  else
  {
    ASSERT(false, ("Could not read VehicleType from string", s));
    vehicleType = VehicleType::Count;
  }
}

std::string DebugPrint(VehicleMask vehicleMask)
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
