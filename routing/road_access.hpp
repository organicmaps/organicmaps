#pragma once

#include "routing/segment.hpp"
#include "routing/vehicle_mask.hpp"

#include "base/assert.hpp"

#include <cstdint>
#include <map>
#include <string>
#include <utility>
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

  std::map<uint32_t, RoadAccess::Type> const & GetFeatureTypes() const { return m_featureTypes; }
  std::map<RoadPoint, RoadAccess::Type> const & GetPointTypes() const { return m_pointTypes; }

  Type GetFeatureType(uint32_t featureId) const;
  Type GetPointType(RoadPoint const & point) const;

  template <typename MF, typename MP>
  void SetAccessTypes(MF && mf, MP && mp)
  {
    m_featureTypes = std::forward<MF>(mf);
    m_pointTypes = std::forward<MP>(mp);
  }

  void Clear();

  void Swap(RoadAccess & rhs);

  bool operator==(RoadAccess const & rhs) const;

  template <typename MF>
  void SetFeatureTypesForTests(MF && mf)
  {
    m_featureTypes = std::forward<MF>(mf);
  }

private:
  // If segmentIdx of a key in this map is 0, it means the
  // entire feature has the corresponding access type.
  // Otherwise, the information is about the segment with number (segmentIdx-1).
  std::map<uint32_t, RoadAccess::Type> m_featureTypes;
  std::map<RoadPoint, RoadAccess::Type> m_pointTypes;
};

std::string ToString(RoadAccess::Type type);
void FromString(std::string const & s, RoadAccess::Type & result);

std::string DebugPrint(RoadAccess::Type type);
std::string DebugPrint(RoadAccess const & r);
}  // namespace routing
