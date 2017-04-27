#pragma once

#include "routing/segment.hpp"
#include "routing/vehicle_mask.hpp"

#include "base/assert.hpp"

#include <cstdint>
#include <map>
#include <string>
#include <vector>

namespace routing
{
// This class provides information about road access classes.
// For now, only restrictive types (such as barrier=gate and access=private)
// and only car routing are supported.
class RoadAccess final
{
public:
  // The road access types are selected by analyzing the most
  // popular tags used when mapping roads in OSM.
  enum class Type : uint8_t
  {
    // Moving through the road is prohibited.
    No,

    // Moving through the road requires a special permission.
    Private,

    // No transit through the road is allowed; however, it can
    // be used if it is close enough to the destination point
    // of the route.
    Destination,

    // No restrictions, as in "access=yes".
    Yes,

    // The number of different road types.
    Count
  };

  RoadAccess();

  static std::vector<VehicleMask> const & GetSupportedVehicleMasks();
  static bool IsSupportedVehicleMask(VehicleMask vehicleMask);

  Type const GetType(VehicleMask vehicleMask, Segment const & segment) const;

  std::map<Segment, Type> const & GetTypes(VehicleMask vehicleMask) const;

  template <typename V>
  void SetTypes(VehicleMask vehicleMask, V && v)
  {
    CHECK(IsSupportedVehicleMask(vehicleMask), ());
    m_types[vehicleMask] = std::forward<V>(v);
  }

  void Clear();

  void Swap(RoadAccess & rhs);

  bool operator==(RoadAccess const & rhs) const;

private:
  // todo(@m) Segment's NumMwmId is not used here. Decouple it from
  // segment and use only (fid, idx, forward) in the map.
  std::map<VehicleMask, std::map<Segment, RoadAccess::Type>> m_types;
};

std::string ToString(RoadAccess::Type type);
void FromString(std::string const & s, RoadAccess::Type & result);

std::string DebugPrint(RoadAccess::Type type);
std::string DebugPrint(RoadAccess const & r);
}  // namespace routing
