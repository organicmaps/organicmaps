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

array<pair<TurnDirection, string>, static_cast<size_t>(TurnDirection::Count)> const g_turnNames =
{{
  { TurnDirection::NoTurn, "NoTurn" },
  { TurnDirection::GoStraight, "GoStraight" },
  { TurnDirection::TurnRight, "TurnRight" },
  { TurnDirection::TurnSharpRight, "TurnSharpRight" },
  { TurnDirection::TurnSlightRight, "TurnSlightRight" },
  { TurnDirection::TurnLeft, "TurnLeft" },
  { TurnDirection::TurnSharpLeft, "TurnSharpLeft" },
  { TurnDirection::TurnSlightLeft, "TurnSlightLeft" },
  { TurnDirection::UTurn, "UTurn" },
  { TurnDirection::TakeTheExit, "TakeTheExit" },
  { TurnDirection::EnterRoundAbout, "EnterRoundAbout" },
  { TurnDirection::LeaveRoundAbout, "LeaveRoundAbout" },
  { TurnDirection::StayOnRoundAbout, "StayOnRoundAbout" },
  { TurnDirection::StartAtEndOfStreet, "StartAtEndOfStreet" },
  { TurnDirection::ReachedYourDestination, "ReachedYourDestination" }
}};
static_assert(g_turnNames.size() == static_cast<size_t>(TurnDirection::Count), "Check the size of g_turnNames");
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

string const GetTurnString(TurnDirection turn)
{
  stringstream out;
  auto const it = find_if(g_turnNames.begin(), g_turnNames.end(),
                          [&turn](pair<TurnDirection, string> const & p)
  {
    return p.first == turn;
  });

  if (it == g_turnNames.end())
    out << "unknown TurnDirection (" << static_cast<int>(turn) << ")";
  out << it->second;
  return out.str();
}

bool IsLeftTurn(TurnDirection t)
{
  return (t >= TurnDirection::TurnLeft && t <= TurnDirection::TurnSlightLeft);
}

bool IsRightTurn(TurnDirection t)
{
  return (t >= TurnDirection::TurnRight && t <= TurnDirection::TurnSlightRight);
}

bool IsLeftOrRightTurn(TurnDirection t)
{
  return IsLeftTurn(t) || IsRightTurn(t);
}

bool IsStayOnRoad(TurnDirection t)
{
  return (t == TurnDirection::GoStraight || t == TurnDirection::StayOnRoundAbout);
}

bool IsGoStraightOrSlightTurn(TurnDirection t)
{
  return (t == TurnDirection::GoStraight || t == TurnDirection::TurnSlightLeft || t == TurnDirection::TurnSlightRight);
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

string DebugPrint(TurnGeom const & turnGeom)
{
  stringstream out;
  out << "[ TurnGeom: m_indexInRoute = " << turnGeom.m_indexInRoute
      << ", m_turnIndex = " << turnGeom.m_turnIndex << " ]" << endl;
  return out.str();
}

string DebugPrint(LaneWay const l)
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

string DebugPrint(TurnDirection const turn)
{
  stringstream out;
  out << "[ " << GetTurnString(turn) << " ]";
  return out.str();
}

}
}
