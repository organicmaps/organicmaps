#pragma once

#include "routing/segment.hpp"
#include "routing/vehicle_mask.hpp"

#include "base/assert.hpp"

#include <cstdint>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "3party/skarupke/flat_hash_map.hpp"

namespace routing
{
// This class provides information about road access classes.
// One instance of RoadAccess holds information about one
// mwm and one router type (also known as VehicleType).
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

    // No pass through the road is allowed; however, it can
    // be used if it is close enough to the destination point
    // of the route.
    Destination,

    // No restrictions, as in "access=yes".
    Yes,

    // The number of different road types.
    Count
  };

  using WayToAccess = ska::flat_hash_map<uint32_t, RoadAccess::Type>;
  using PointToAccess = ska::flat_hash_map<RoadPoint, RoadAccess::Type, RoadPoint::Hash>;

  WayToAccess const & GetWayToAccess() const
  {
    return m_wayToAccess;
  }

  PointToAccess const & GetPointToAccess() const
  {
    return m_pointToAccess;
  }

  Type GetAccess(uint32_t featureId) const;
  Type GetAccess(RoadPoint const & point) const;

  template <typename WayToAccess, typename PointToAccess>
  void SetAccess(WayToAccess && wayToAccess, PointToAccess && pointToAccess)
  {
    m_wayToAccess = std::forward<WayToAccess>(wayToAccess);
    m_pointToAccess = std::forward<PointToAccess>(pointToAccess);
  }

  bool operator==(RoadAccess const & rhs) const;

  template <typename WayToAccess>
  void SetWayToAccessForTests(WayToAccess && wayToAccess)
  {
    m_wayToAccess = std::forward<WayToAccess>(wayToAccess);
  }

private:
  // If segmentIdx of a key in this map is 0, it means the
  // entire feature has the corresponding access type.
  // Otherwise, the information is about the segment with number (segmentIdx-1).
  WayToAccess m_wayToAccess;
  PointToAccess m_pointToAccess;
};

std::string ToString(RoadAccess::Type type);
void FromString(std::string const & s, RoadAccess::Type & result);

std::string DebugPrint(RoadAccess::Type type);
std::string DebugPrint(RoadAccess const & r);
}  // namespace routing
