#include "routing/road_access.hpp"

#include <algorithm>
#include <sstream>

using namespace std;

namespace
{
string const kNo = "No";
string const kPrivate = "Private";
string const kDestination = "Destination";
string const kYes = "Yes";
string const kCount = "Count";
string const kNames[] = {kNo, kPrivate, kDestination, kYes, kCount};

// *NOTE* Order may be important for users (such as serializers).
vector<routing::RouterType> const kSupportedRouterTypes = {
  routing::RouterType::Vehicle,
  routing::RouterType::Pedestrian,
  routing::RouterType::Bicycle,
};
}  // namespace

namespace routing
{
// RoadAccess --------------------------------------------------------------------------------------
// static
vector<RouterType> const & RoadAccess::GetSupportedRouterTypes() { return kSupportedRouterTypes; }

// static
bool RoadAccess::IsSupportedRouterType(RouterType r)
{
  return find(kSupportedRouterTypes.begin(), kSupportedRouterTypes.end(), r) !=
         kSupportedRouterTypes.end();
}

RoadAccess::Type const RoadAccess::GetType(RouterType routerType, Segment const & segment) const
{
  auto const & types = GetTypes(routerType);
  Segment key(kFakeNumMwmId, segment.GetFeatureId(), segment.GetSegmentIdx(), segment.IsForward());
  auto const it = types.find(key);
  if (it != types.end())
    return it->second;

  return RoadAccess::Type::Yes;
}

map<Segment, RoadAccess::Type> const & RoadAccess::GetTypes(RouterType routerType) const
{
  ASSERT(IsSupportedRouterType(routerType), ());
  return m_types[static_cast<size_t>(routerType)];
}

void RoadAccess::Clear()
{
  for (size_t i = 0; i < static_cast<size_t>(RouterType::Count); ++i)
    m_types[i].clear();
}

void RoadAccess::Swap(RoadAccess & rhs)
{
  for (size_t i = 0; i < static_cast<size_t>(RouterType::Count); ++i)
    m_types[i].swap(rhs.m_types[i]);
}

bool RoadAccess::operator==(RoadAccess const & rhs) const
{
  for (size_t i = 0; i < static_cast<size_t>(RouterType::Count); ++i)
  {
    if (m_types[i] != rhs.m_types[i])
      return false;
  }
  return true;
}

// Functions ---------------------------------------------------------------------------------------
string ToString(RoadAccess::Type type)
{
  if (type <= RoadAccess::Type::Count)
    return kNames[static_cast<size_t>(type)];
  ASSERT(false, ());
  return {};
}

void FromString(std::string const & s, RoadAccess::Type & result)
{
  for (size_t i = 0; i < static_cast<size_t>(RoadAccess::Type::Count); ++i)
  {
    if (s == kNames[i])
    {
      result = static_cast<RoadAccess::Type>(i);
      return;
    }
  }
  ASSERT(false, ());
}

string DebugPrint(RoadAccess::Type type) { return ToString(type); }

string DebugPrint(RoadAccess const & r)
{
  size_t const kMaxIdsToShow = 10;
  ostringstream oss;
  oss << "RoadAccess [";
  for (size_t i = 0; i < static_cast<size_t>(RouterType::Count); ++i)
  {
    if (i > 0)
      oss << ", ";
    auto const routerType = static_cast<RouterType>(i);
    oss << DebugPrint(routerType) << " [";
    size_t id = 0;
    for (auto const & kv : r.GetTypes(routerType))
    {
      if (id > 0)
        oss << ", ";
      oss << DebugPrint(kv.first) << " " << DebugPrint(kv.second);
      ++id;
      if (id == kMaxIdsToShow)
        break;
    }
    if (r.GetTypes(routerType).size() > kMaxIdsToShow)
      oss << "...";
    oss << "]";
  }
  oss << "]";
  return oss.str();
}
}  // namespace routing
