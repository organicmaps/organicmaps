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

  std::map<Segment, RoadAccess::Type> const & GetSegmentTypes() const { return m_segmentTypes; }

  Type GetSegmentType(Segment const & segment) const;

  template <typename V>
  void SetSegmentTypes(V && v)
  {
    m_segmentTypes = std::forward<V>(v);
  }

  void Clear();

  void Swap(RoadAccess & rhs);

  bool operator==(RoadAccess const & rhs) const;

private:
  // todo(@m) Segment's NumMwmId is not used here. Decouple it from
  // segment and use only (fid, idx, forward) in the map.
  //
  // If segmentIdx of a key in this map is 0, it means the
  // entire feature has the corresponding access type.
  // Otherwise, the information is about the segment with number (segmentIdx-1).
  std::map<Segment, RoadAccess::Type> m_segmentTypes;
};

std::string ToString(RoadAccess::Type type);
void FromString(std::string const & s, RoadAccess::Type & result);

std::string DebugPrint(RoadAccess::Type type);
std::string DebugPrint(RoadAccess const & r);
}  // namespace routing
