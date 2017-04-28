#include "routing/road_access.hpp"

#include "base/assert.hpp"

#include <algorithm>
#include <sstream>

using namespace std;

namespace
{
string const kNames[] = {"No", "Private", "Destination", "Yes", "Count"};
}  // namespace

namespace routing
{
// RoadAccess --------------------------------------------------------------------------------------
RoadAccess::RoadAccess() : m_vehicleType(VehicleType::Count) {}

RoadAccess::RoadAccess(VehicleType vehicleType) : m_vehicleType(vehicleType) {}

RoadAccess::Type const RoadAccess::GetSegmentType(Segment const & segment) const
{
  Segment key(kFakeNumMwmId, segment.GetFeatureId(), segment.GetSegmentIdx(), segment.IsForward());
  auto const it = m_segmentTypes.find(key);
  if (it != m_segmentTypes.end())
    return it->second;

  return RoadAccess::Type::Yes;
}

bool RoadAccess::operator==(RoadAccess const & rhs) const
{
  return m_segmentTypes == rhs.m_segmentTypes;
}

// Functions ---------------------------------------------------------------------------------------
string ToString(RoadAccess::Type type)
{
  if (type <= RoadAccess::Type::Count)
    return kNames[static_cast<size_t>(type)];
  ASSERT(false, ("Bad road access type", static_cast<size_t>(type)));
  return "Bad RoadAccess::Type";
}

void FromString(string const & s, RoadAccess::Type & result)
{
  for (size_t i = 0; i <= static_cast<size_t>(RoadAccess::Type::Count); ++i)
  {
    if (s == kNames[i])
    {
      result = static_cast<RoadAccess::Type>(i);
      return;
    }
  }
  result = RoadAccess::Type::Count;
  ASSERT(false, ("Could not read RoadAccess from the string", s));
}

string DebugPrint(RoadAccess::Type type) { return ToString(type); }

string DebugPrint(RoadAccess const & r)
{
  size_t const kMaxIdsToShow = 10;
  ostringstream oss;
  oss << "RoadAccess " << DebugPrint(r.GetVehicleType()) << " [";
  size_t id = 0;
  for (auto const & kv : r.GetSegmentTypes())
  {
    if (id > 0)
      oss << ", ";
    oss << DebugPrint(kv.first) << " " << DebugPrint(kv.second);
    ++id;
    if (id == kMaxIdsToShow)
      break;
  }
  if (r.GetSegmentTypes().size() > kMaxIdsToShow)
    oss << ", ...";

  oss << "]";
  return oss.str();
}
}  // namespace routing
