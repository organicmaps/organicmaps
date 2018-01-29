#include "routing/turns.hpp"

#include "geometry/angles.hpp"

#include "platform/country_file.hpp"

#include "base/internal/message.hpp"
#include "base/stl_helpers.hpp"
#include "base/string_utils.hpp"

#include <algorithm>
#include <array>
#include <sstream>
#include <utility>

using namespace std;

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

array<pair<CarDirection, char const *>, static_cast<size_t>(CarDirection::Count)> const g_turnNames = {
    {{CarDirection::None, "None"},
     {CarDirection::GoStraight, "GoStraight"},
     {CarDirection::TurnRight, "TurnRight"},
     {CarDirection::TurnSharpRight, "TurnSharpRight"},
     {CarDirection::TurnSlightRight, "TurnSlightRight"},
     {CarDirection::TurnLeft, "TurnLeft"},
     {CarDirection::TurnSharpLeft, "TurnSharpLeft"},
     {CarDirection::TurnSlightLeft, "TurnSlightLeft"},
     {CarDirection::UTurnLeft, "UTurnLeft"},
     {CarDirection::UTurnRight, "UTurnRight"},
     {CarDirection::TakeTheExit, "TakeTheExit"},
     {CarDirection::EnterRoundAbout, "EnterRoundAbout"},
     {CarDirection::LeaveRoundAbout, "LeaveRoundAbout"},
     {CarDirection::StayOnRoundAbout, "StayOnRoundAbout"},
     {CarDirection::StartAtEndOfStreet, "StartAtEndOfStreet"},
     {CarDirection::ReachedYourDestination, "ReachedYourDestination"}}};
static_assert(g_turnNames.size() == static_cast<size_t>(CarDirection::Count),
              "Check the size of g_turnNames");
}  // namespace

namespace routing
{
// SegmentRange -----------------------------------------------------------------------------------
SegmentRange::SegmentRange(FeatureID const & featureId, uint32_t startSegId, uint32_t endSegId,
                     bool forward)
  : m_featureId(featureId)
  , m_startSegId(startSegId)
  , m_endSegId(endSegId)
  , m_forward(forward)
{
}

bool SegmentRange::operator==(SegmentRange const & rhs) const
{
  return m_featureId == rhs.m_featureId && m_startSegId == rhs.m_startSegId &&
         m_endSegId == rhs.m_endSegId && m_forward == rhs.m_forward;
}

bool SegmentRange::operator<(SegmentRange const & rhs) const
{
  if (m_featureId != rhs.m_featureId)
    return m_featureId < rhs.m_featureId;

  if (m_startSegId != rhs.m_startSegId)
    return m_startSegId < rhs.m_startSegId;

  if (m_endSegId != rhs.m_endSegId)
    return m_endSegId < rhs.m_endSegId;

  return m_forward < rhs.m_forward;
}

void SegmentRange::Clear()
{
  m_featureId = FeatureID();
  m_startSegId = 0;
  m_endSegId = 0;
  m_forward = true;
}

bool SegmentRange::IsEmpty() const
{
  return !m_featureId.IsValid() && m_startSegId == 0 && m_endSegId == 0 && m_forward;
}

FeatureID const & SegmentRange::GetFeature() const
{
  return m_featureId;
}

bool SegmentRange::IsCorrect() const
{
  return (m_forward && m_startSegId <= m_endSegId) || (!m_forward && m_endSegId <= m_startSegId);
}

Segment SegmentRange::GetFirstSegment(NumMwmIds const & numMwmIds) const
{
  if (!m_featureId.IsValid())
    return Segment();

  return Segment(numMwmIds.GetId(platform::CountryFile(m_featureId.GetMwmName())),
                 m_featureId.m_index, m_startSegId, m_forward);
}

string DebugPrint(SegmentRange const & segmentRange)
{
  stringstream out;
  out << "SegmentRange [ m_featureId = " << DebugPrint(segmentRange.m_featureId)
      << ", m_startSegId = " << segmentRange.m_startSegId
      << ", m_endSegId = " << segmentRange.m_endSegId
      << ", m_forward = " << segmentRange.m_forward
      << "]" << endl;
  return out.str();
}

namespace turns
{
// SingleLaneInfo ---------------------------------------------------------------------------------
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

string const GetTurnString(CarDirection turn)
{
  for (auto const & p : g_turnNames)
  {
    if (p.first == turn)
      return p.second;
  }

  stringstream out;
  out << "unknown CarDirection (" << static_cast<int>(turn) << ")";
  return out.str();
}

bool IsLeftTurn(CarDirection t)
{
  return (t >= CarDirection::TurnLeft && t <= CarDirection::TurnSlightLeft);
}

bool IsRightTurn(CarDirection t)
{
  return (t >= CarDirection::TurnRight && t <= CarDirection::TurnSlightRight);
}

bool IsLeftOrRightTurn(CarDirection t)
{
  return IsLeftTurn(t) || IsRightTurn(t);
}

bool IsStayOnRoad(CarDirection t)
{
  return (t == CarDirection::GoStraight || t == CarDirection::StayOnRoundAbout);
}

bool IsGoStraightOrSlightTurn(CarDirection t)
{
  return (t == CarDirection::GoStraight || t == CarDirection::TurnSlightLeft ||
          t == CarDirection::TurnSlightRight);
}

bool IsLaneWayConformedTurnDirection(LaneWay l, CarDirection t)
{
  switch (t)
  {
    default:
      return false;
    case CarDirection::GoStraight:
      return l == LaneWay::Through;
    case CarDirection::TurnRight:
      return l == LaneWay::Right;
    case CarDirection::TurnSharpRight:
      return l == LaneWay::SharpRight;
    case CarDirection::TurnSlightRight:
      return l == LaneWay::SlightRight;
    case CarDirection::TurnLeft:
      return l == LaneWay::Left;
    case CarDirection::TurnSharpLeft:
      return l == LaneWay::SharpLeft;
    case CarDirection::TurnSlightLeft:
      return l == LaneWay::SlightLeft;
    case CarDirection::UTurnLeft:
    case CarDirection::UTurnRight:
      return l == LaneWay::Reverse;
  }
}

bool IsLaneWayConformedTurnDirectionApproximately(LaneWay l, CarDirection t)
{
  switch (t)
  {
    default:
      return false;
    case CarDirection::GoStraight:
      return l == LaneWay::Through || l == LaneWay::SlightRight || l == LaneWay::SlightLeft;
    case CarDirection::TurnRight:
      return l == LaneWay::Right || l == LaneWay::SharpRight || l == LaneWay::SlightRight;
    case CarDirection::TurnSharpRight:
      return l == LaneWay::SharpRight || l == LaneWay::Right;
    case CarDirection::TurnSlightRight:
      return l == LaneWay::SlightRight || l == LaneWay::Through || l == LaneWay::Right;
    case CarDirection::TurnLeft:
      return l == LaneWay::Left || l == LaneWay::SlightLeft || l == LaneWay::SharpLeft;
    case CarDirection::TurnSharpLeft:
      return l == LaneWay::SharpLeft || l == LaneWay::Left;
    case CarDirection::TurnSlightLeft:
      return l == LaneWay::SlightLeft || l == LaneWay::Through || l == LaneWay::Left;
    case CarDirection::UTurnLeft:
    case CarDirection::UTurnRight:
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
  strings::AsciiToLower(lanesString);
  my::EraseIf(lanesString, [](char c) { return isspace(c); });

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

string DebugPrint(CarDirection const turn)
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
