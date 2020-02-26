#include "routing/road_access.hpp"

#include "base/assert.hpp"

#include <algorithm>
#include <chrono>
#include <sstream>

using namespace std;

namespace
{
string const kNames[] = {"No", "Private", "Destination", "Yes", "Count"};

template <typename KV>
void PrintKV(ostringstream & oss, KV const & kvs, size_t maxKVToShow)
{
  size_t i = 0;
  for (auto const & kv : kvs)
  {
    if (i > 0)
      oss << ", ";
    oss << DebugPrint(kv.first) << " " << DebugPrint(kv.second);
    ++i;
    if (i == maxKVToShow)
      break;
  }
  if (kvs.size() > maxKVToShow)
    oss << ", ...";
}
}  // namespace

namespace routing
{
// RoadAccess --------------------------------------------------------------------------------------
RoadAccess::RoadAccess() : m_currentTimeGetter([](){ return GetCurrentTimestamp(); })
{
}

RoadAccess::Type RoadAccess::GetAccess(uint32_t featureId) const
{
// todo(@m) This may or may not be too slow. Consider profiling this and using
// a Bloom filter or anything else that is faster than std::map.
  auto const it = m_wayToAccess.find(featureId);
  if (it != m_wayToAccess.cend())
    return it->second;

  return RoadAccess::Type::Yes;
}

RoadAccess::Type RoadAccess::GetAccess(RoadPoint const & point) const
{
  auto const it = m_pointToAccess.find(point);
  if (it != m_pointToAccess.cend())
    return it->second;

  return RoadAccess::Type::Yes;
}

bool RoadAccess::operator==(RoadAccess const & rhs) const
{
  return m_wayToAccess == rhs.m_wayToAccess && m_pointToAccess == rhs.m_pointToAccess &&
         m_wayToAccessConditional == rhs.m_wayToAccessConditional &&
         m_pointToAccessConditional == rhs.m_pointToAccessConditional;
}

// Functions ---------------------------------------------------------------------------------------
time_t GetCurrentTimestamp()
{
  using system_clock = std::chrono::system_clock;
  return system_clock::to_time_t(system_clock::now());
}

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

string DebugPrint(RoadAccess::Conditional const & conditional)
{
  stringstream ss;
  ss << " { ";
  for (auto const & access : conditional.GetAccesses())
  {
    ss << DebugPrint(access.m_type) << " @ (" << access.m_openingHours.GetRule() << "), ";
  }
  ss << " } ";
  return ss.str();
}

string DebugPrint(RoadAccess::Type type) { return ToString(type); }

string DebugPrint(RoadAccess const & r)
{
  size_t const kMaxIdsToShow = 10;
  ostringstream oss;
  oss << "WayToAccess { FeatureTypes [";
  PrintKV(oss, r.GetWayToAccess(), kMaxIdsToShow);
  oss << "], PointToAccess [";
  PrintKV(oss, r.GetPointToAccess(), kMaxIdsToShow);
  oss << "], WayToAccessConditional [";
  PrintKV(oss, r.GetWayToAccessConditional(), kMaxIdsToShow);
  oss << "], PointToAccessConditional [";
  PrintKV(oss, r.GetPointToAccessConditional(), kMaxIdsToShow);
  oss << "] }";
  return oss.str();
}
}  // namespace routing
