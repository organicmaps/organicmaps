#include "routing/turns.hpp"

#include "std/array.hpp"


namespace
{
using namespace routing::turns;
// The order is important. Starting with the most frequent tokens according to
// taginfo.openstreetmap.org to minimize the number of comparisons in ParseSingleLane().
array<pair<LaneWay, string>, static_cast<size_t>(LaneWay::Count)> const g_laneWayNames =
{{
  { LaneWay::Through, "through" },
  { LaneWay::Left, "left" },
  { LaneWay::Right, "right" },
  { LaneWay::None, "none" },
  { LaneWay::SharpLeft, "sharp_left" },
  { LaneWay::SlightLeft, "slight_left" },
  { LaneWay::MergeToRight, "merge_to_right" },
  { LaneWay::MergeToLeft, "merge_to_left" },
  { LaneWay::SlightRight, "slight_right" },
  { LaneWay::SharpRight, "sharp_right" },
  { LaneWay::Reverse, "reverse" }
}};
static_assert(g_laneWayNames.size() == static_cast<size_t>(LaneWay::Count), "Check the size of g_laneWayNames");
}

namespace routing
{
namespace turns
{
bool TurnGeom::operator==(TurnGeom const & other) const
{
  return m_indexInRoute == other.m_indexInRoute && m_turnIndex == other.m_turnIndex
    && m_points == other.m_points;
}

string turnStrings[] = {
  "NoTurn",
  "GoStraight",
  "TurnRight",
  "TurnSharpRight",
  "TurnSlightRight",
  "TurnLeft",
  "TurnSharpLeft",
  "TurnSlightLeft",
  "UTurn",

  "TakeTheExit",

  "EnterRoundAbout",
  "LeaveRoundAbout",
  "StayOnRoundAbout",

  "StartAtEndOfStreet",
  "ReachedYourDestination"
};

string const & GetTurnString(TurnDirection turn)
{
  return turnStrings[turn];
}

bool IsLeftTurn(TurnDirection t)
{
  return (t >= turns::TurnLeft && t <= turns::TurnSlightLeft);
}

bool IsRightTurn(TurnDirection t)
{
  return (t >= turns::TurnRight && t <= turns::TurnSlightRight);
}

bool IsLeftOrRightTurn(TurnDirection t)
{
  return IsLeftTurn(t) || IsRightTurn(t);
}

bool IsStayOnRoad(TurnDirection t)
{
  return (t == turns::GoStraight || t == turns::StayOnRoundAbout);
}

bool IsGoStraightOrSlightTurn(TurnDirection t)
{
  return (t == turns::GoStraight || t == turns::TurnSlightLeft || t == turns::TurnSlightRight);
}

void SplitLanes(string const & lanesString, char delimiter, vector<string> & lanes)
{
  lanes.clear();
  istringstream lanesStream(lanesString);
  string token;
  while (getline(lanesStream, token, delimiter))
  {
    lanes.push_back(token);
  }
}

bool ParseSingleLane(string const & laneString, char delimiter, TSingleLane & lane)
{
  lane.clear();
  istringstream laneStream(laneString);
  string token;
  while (getline(laneStream, token, delimiter))
  {
    auto const it = find_if(g_laneWayNames.begin(), g_laneWayNames.end(),
                            [&token](pair<LaneWay, string> const & p)
    {
        return p.second == token;
    });
    if (it == g_laneWayNames.end())
      return false;
    lane.push_back(it->first);
  }
  return true;
}

bool ParseLanes(string lanesString, vector<TSingleLane> & lanes)
{
  if (lanesString.empty())
    return false;
  lanes.clear();
  transform(lanesString.begin(), lanesString.end(), lanesString.begin(), tolower);
  lanesString.erase(remove_if(lanesString.begin(), lanesString.end(), isspace),
                         lanesString.end());

  vector<string> SplitLanesStrings;
  TSingleLane lane;
  SplitLanes(lanesString, '|', SplitLanesStrings);
  for (string const & s : SplitLanesStrings)
  {
    if (!ParseSingleLane(s, ';', lane))
    {
      lanes.clear();
      return false;
    }
    lanes.push_back(lane);
  }
  return true;
}

string DebugPrint(routing::turns::TurnGeom const & turnGeom)
{
  stringstream out;
  out << "[ TurnGeom: m_indexInRoute = " << turnGeom.m_indexInRoute
      << ", m_turnIndex = " << turnGeom.m_turnIndex << " ]" << endl;
  return out.str();
}

string DebugPrint(routing::turns::LaneWay const l)
{
  stringstream out;
  auto const it = find_if(g_laneWayNames.begin(), g_laneWayNames.end(),
                          [&l](pair<LaneWay, string> const & p)
  {
    return p.first == l;
  });

  if (it == g_laneWayNames.end())
    out << "unknown LaneWay (" << static_cast<int>(l) << ")";
  out << it->second;
  return out.str();
}

}
}
