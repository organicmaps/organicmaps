#include "routing/turns.hpp"

#include "geometry/angles.hpp"

#include "base/internal/message.hpp"

#include "std/array.hpp"
#include "std/utility.hpp"

namespace
{
using namespace routing::turns;

/// The order is important. Starting with the most frequent tokens according to
/// taginfo.openstreetmap.org we minimize the number of the comparisons in ParseSingleLane().
array<pair<LaneWay, char const *>, static_cast<size_t>(LaneWay::Count)> const g_laneWayNames = {
    {{LaneWay::Through, "through"},
     {LaneWay::Left, "left"},
     {LaneWay::Right, "right"},
     {LaneWay::None, "none"},
     {LaneWay::SharpLeft, "sharp_left"},
     {LaneWay::SlightLeft, "slight_left"},
     {LaneWay::MergeToRight, "merge_to_right"},
     {LaneWay::MergeToLeft, "merge_to_left"},
     {LaneWay::SlightRight, "slight_right"},
     {LaneWay::SharpRight, "sharp_right"},
     {LaneWay::Reverse, "reverse"}}};
static_assert(g_laneWayNames.size() == static_cast<size_t>(LaneWay::Count),
              "Check the size of g_laneWayNames");

array<pair<TurnDirection, char const *>, static_cast<size_t>(TurnDirection::Count)> const g_turnNames = {
    {{TurnDirection::NoTurn, "NoTurn"},
     {TurnDirection::GoStraight, "GoStraight"},
     {TurnDirection::TurnRight, "TurnRight"},
     {TurnDirection::TurnSharpRight, "TurnSharpRight"},
     {TurnDirection::TurnSlightRight, "TurnSlightRight"},
     {TurnDirection::TurnLeft, "TurnLeft"},
     {TurnDirection::TurnSharpLeft, "TurnSharpLeft"},
     {TurnDirection::TurnSlightLeft, "TurnSlightLeft"},
     {TurnDirection::UTurnLeft, "UTurnLeft"},
     {TurnDirection::UTurnRight, "UTurnRight"},
     {TurnDirection::TakeTheExit, "TakeTheExit"},
     {TurnDirection::EnterRoundAbout, "EnterRoundAbout"},
     {TurnDirection::LeaveRoundAbout, "LeaveRoundAbout"},
     {TurnDirection::StayOnRoundAbout, "StayOnRoundAbout"},
     {TurnDirection::StartAtEndOfStreet, "StartAtEndOfStreet"},
     {TurnDirection::ReachedYourDestination, "ReachedYourDestination"}}};
static_assert(g_turnNames.size() == static_cast<size_t>(TurnDirection::Count),
              "Check the size of g_turnNames");
}  // namespace

namespace routing
{
// UniNodeId -------------------------------------------------------------------
bool UniNodeId::operator==(UniNodeId const & rhs) const
{
  if (m_type != rhs.m_type)
    return false;

  switch (m_type)
  {
  case Type::Osrm: return m_nodeId == rhs.m_nodeId;
  case Type::Mwm:
    return m_featureId == rhs.m_featureId && m_segId == rhs.m_segId && m_forward == rhs.m_forward;
  }
}

bool UniNodeId::operator<(UniNodeId const & rhs) const
{
  if (m_type != rhs.m_type)
    return m_type < rhs.m_type;

  switch (m_type)
  {
  case Type::Osrm: return m_nodeId < rhs.m_nodeId;
  case Type::Mwm:
    if (m_featureId != rhs.m_featureId)
      return m_featureId < rhs.m_featureId;

    if (m_segId != rhs.m_segId)
      return m_segId < rhs.m_segId;

    return m_forward < rhs.m_forward;
  }
}

void UniNodeId::Clear()
{
  m_featureId = FeatureID();
  m_segId = 0;
  m_forward = true;
  m_nodeId = SPECIAL_NODEID;
}

uint32_t UniNodeId::GetNodeId() const
{
  ASSERT_EQUAL(m_type, Type::Osrm, ());
  return m_nodeId;
}

FeatureID const & UniNodeId::GetFeature() const
{
  ASSERT_EQUAL(m_type, Type::Mwm, ());
  return m_featureId;
}

uint32_t UniNodeId::GetSegId() const
{
  ASSERT_EQUAL(m_type, Type::Mwm, ());
  return m_segId;
}

bool UniNodeId::IsForward() const
{
  ASSERT_EQUAL(m_type, Type::Mwm, ());
  return m_forward;
}

string DebugPrint(UniNodeId::Type type)
{
  switch (type)
  {
  case UniNodeId::Type::Osrm: return "Osrm";
  case UniNodeId::Type::Mwm: return "Mwm";
  }
}

namespace turns
{
// SingleLaneInfo --------------------------------------------------------------
bool SingleLaneInfo::operator==(SingleLaneInfo const & other) const
{
  return m_lane == other.m_lane && m_isRecommended == other.m_isRecommended;
}

string DebugPrint(TurnItem const & turnItem)
{
  stringstream out;
  out << "TurnItem [ m_index = " << turnItem.m_index
      << ", m_turn = " << DebugPrint(turnItem.m_turn)
      << ", m_lanes = " << ::DebugPrint(turnItem.m_lanes) << ", m_exitNum = " << turnItem.m_exitNum
      << ", m_sourceName = " << turnItem.m_sourceName
      << ", m_targetName = " << turnItem.m_targetName
      << ", m_keepAnyway = " << turnItem.m_keepAnyway
      << ", m_pedestrianDir = " << DebugPrint(turnItem.m_pedestrianTurn)
      << " ]" << endl;
  return out.str();
}

string DebugPrint(TurnItemDist const & turnItemDist)
{
  stringstream out;
  out << "TurnItemDist [ m_turnItem = " << DebugPrint(turnItemDist.m_turnItem)
      << ", m_distMeters = " << turnItemDist.m_distMeters
      << " ]" << endl;
  return out.str();
}

string const GetTurnString(TurnDirection turn)
{
  for (auto const & p : g_turnNames)
  {
    if (p.first == turn)
      return p.second;
  }

  stringstream out;
  out << "unknown TurnDirection (" << static_cast<int>(turn) << ")";
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
  return (t == TurnDirection::GoStraight || t == TurnDirection::TurnSlightLeft ||
          t == TurnDirection::TurnSlightRight);
}

bool IsLaneWayConformedTurnDirection(LaneWay l, TurnDirection t)
{
  switch (t)
  {
    default:
      return false;
    case TurnDirection::GoStraight:
      return l == LaneWay::Through;
    case TurnDirection::TurnRight:
      return l == LaneWay::Right;
    case TurnDirection::TurnSharpRight:
      return l == LaneWay::SharpRight;
    case TurnDirection::TurnSlightRight:
      return l == LaneWay::SlightRight;
    case TurnDirection::TurnLeft:
      return l == LaneWay::Left;
    case TurnDirection::TurnSharpLeft:
      return l == LaneWay::SharpLeft;
    case TurnDirection::TurnSlightLeft:
      return l == LaneWay::SlightLeft;
    case TurnDirection::UTurnLeft:
    case TurnDirection::UTurnRight:
      return l == LaneWay::Reverse;
  }
}

bool IsLaneWayConformedTurnDirectionApproximately(LaneWay l, TurnDirection t)
{
  switch (t)
  {
    default:
      return false;
    case TurnDirection::GoStraight:
      return l == LaneWay::Through || l == LaneWay::SlightRight || l == LaneWay::SlightLeft;
    case TurnDirection::TurnRight:
      return l == LaneWay::Right || l == LaneWay::SharpRight || l == LaneWay::SlightRight;
    case TurnDirection::TurnSharpRight:
      return l == LaneWay::SharpRight || l == LaneWay::Right;
    case TurnDirection::TurnSlightRight:
      return l == LaneWay::SlightRight || l == LaneWay::Through || l == LaneWay::Right;
    case TurnDirection::TurnLeft:
      return l == LaneWay::Left || l == LaneWay::SlightLeft || l == LaneWay::SharpLeft;
    case TurnDirection::TurnSharpLeft:
      return l == LaneWay::SharpLeft || l == LaneWay::Left;
    case TurnDirection::TurnSlightLeft:
      return l == LaneWay::SlightLeft || l == LaneWay::Through || l == LaneWay::Left;
    case TurnDirection::UTurnLeft:
    case TurnDirection::UTurnRight:
      return l == LaneWay::Reverse;
  }
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

bool ParseLanes(string lanesString, vector<SingleLaneInfo> & lanes)
{
  if (lanesString.empty())
    return false;
  lanes.clear();
  transform(lanesString.begin(), lanesString.end(), lanesString.begin(), tolower);
  lanesString.erase(remove_if(lanesString.begin(), lanesString.end(), isspace),
                         lanesString.end());

  vector<string> SplitLanesStrings;
  SingleLaneInfo lane;
  SplitLanes(lanesString, '|', SplitLanesStrings);
  for (string const & s : SplitLanesStrings)
  {
    if (!ParseSingleLane(s, ';', lane.m_lane))
    {
      lanes.clear();
      return false;
    }
    lanes.push_back(lane);
  }
  return true;
}

string DebugPrint(LaneWay const l)
{
  auto const it = find_if(g_laneWayNames.begin(), g_laneWayNames.end(),
                          [&l](pair<LaneWay, string> const & p)
  {
    return p.first == l;
  });

  if (it == g_laneWayNames.end())
  {
    stringstream out;
    out << "unknown LaneWay (" << static_cast<int>(l) << ")";
    return out.str();
  }
  return it->second;
}

string DebugPrint(TurnDirection const turn)
{
  stringstream out;
  out << "[ " << GetTurnString(turn) << " ]";
  return out.str();
}

string DebugPrint(PedestrianDirection const l)
{
  switch (l)
  {
  case PedestrianDirection::None: return "None";
  case PedestrianDirection::Upstairs: return "Upstairs";
  case PedestrianDirection::Downstairs: return "Downstairs";
  case PedestrianDirection::LiftGate: return "LiftGate";
  case PedestrianDirection::Gate: return "Gate";
  case PedestrianDirection::ReachedYourDestination: return "ReachedYourDestination";
  case PedestrianDirection::Count:
    // PedestrianDirection::Count should be never used in the code, print it as unknown value
    // (it is added to cases list to suppress compiler warning).
    break;
  }

  stringstream out;
  out << "unknown PedestrianDirection (" << static_cast<int>(l) << ")";
  return out.str();
}

string DebugPrint(SingleLaneInfo const & singleLaneInfo)
{
  stringstream out;
  out << "SingleLaneInfo [ m_isRecommended == " << singleLaneInfo.m_isRecommended
      << ", m_lane == " << ::DebugPrint(singleLaneInfo.m_lane) << " ]" << endl;
  return out.str();
}

double PiMinusTwoVectorsAngle(m2::PointD const & junctionPoint, m2::PointD const & ingoingPoint,
                              m2::PointD const & outgoingPoint)
{
  return math::pi - ang::TwoVectorsAngle(junctionPoint, ingoingPoint, outgoingPoint);
}
}  // namespace turns
}  // namespace routing
