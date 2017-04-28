#include "routing/road_access.hpp"

#include "base/assert.hpp"

#include <algorithm>
#include <sstream>

using namespace std;

namespace
{
string const kNames[] = {"No", "Private", "Destination", "Yes", "Count"};

// *NOTE* Order may be important for users (such as serializers).
// Add new types to the end of the list.
vector<routing::VehicleMask> const kSupportedVehicleMasks = {
  routing::kPedestrianMask,
  routing::kBicycleMask,
  routing::kCarMask,
};  
}  // namespace

namespace routing
{
// RoadAccess --------------------------------------------------------------------------------------
RoadAccess::RoadAccess() : m_vehicleMask(kAllVehiclesMask) {}

RoadAccess::RoadAccess(VehicleMask vehicleMask) : m_vehicleMask(vehicleMask)
{
  CHECK(IsSupportedVehicleMask(vehicleMask), ());
}

// static
vector<VehicleMask> const & RoadAccess::GetSupportedVehicleMasks()
{
  return kSupportedVehicleMasks;
}

// static
bool RoadAccess::IsSupportedVehicleMask(VehicleMask vehicleMask)
{
  return find(kSupportedVehicleMasks.begin(), kSupportedVehicleMasks.end(), vehicleMask) !=
         kSupportedVehicleMasks.end();
}

RoadAccess::Type const RoadAccess::GetType(Segment const & segment) const
{
  Segment key(kFakeNumMwmId, segment.GetFeatureId(), segment.GetSegmentIdx(), segment.IsForward());
  auto const it = m_types.find(key);
  if (it != m_types.end())
    return it->second;

  return RoadAccess::Type::Yes;
}

void RoadAccess::Clear()
{
  m_types.clear();
}

void RoadAccess::Swap(RoadAccess & rhs)
{
  m_types.swap(rhs.m_types);
}

bool RoadAccess::operator==(RoadAccess const & rhs) const
{
  return m_types == rhs.m_types;
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
  oss << "RoadAccess " << DebugPrint(r.GetVehicleMask()) << "[ ";
  size_t id = 0;
  for (auto const & kv : r.GetTypes())
  {
    if (id > 0)
      oss << ", ";
    oss << DebugPrint(kv.first) << " " << DebugPrint(kv.second);
    ++id;
    if (id == kMaxIdsToShow)
      break;
  }
  if (r.GetTypes().size() > kMaxIdsToShow)
    oss << "...";

  oss << "]";
  return oss.str();
}
}  // namespace routing
