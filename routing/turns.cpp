#include "routing/turns.hpp"

#include "geometry/angles.hpp"

#include "platform/country_file.hpp"

#include "base/internal/message.hpp"
#include "base/stl_helpers.hpp"
#include "base/string_utils.hpp"

#include <sstream>
#include <utility>

namespace routing
{
using namespace routing::turns;
using namespace std;

// SegmentRange -----------------------------------------------------------------------------------
SegmentRange::SegmentRange(FeatureID const & featureId, uint32_t startSegId, uint32_t endSegId,
                           bool forward, m2::PointD const & start, m2::PointD const & end)
  : m_featureId(featureId), m_startSegId(startSegId), m_endSegId(endSegId), m_forward(forward),
    m_start(start), m_end(end)
{
  if (m_startSegId != m_endSegId)
    CHECK_EQUAL(m_forward, m_startSegId < m_endSegId, (*this));
}

bool SegmentRange::operator==(SegmentRange const & rhs) const
{
  return m_featureId == rhs.m_featureId && m_startSegId == rhs.m_startSegId &&
         m_endSegId == rhs.m_endSegId && m_forward == rhs.m_forward && m_start == rhs.m_start &&
         m_end == rhs.m_end;
}

bool SegmentRange::operator<(SegmentRange const & rhs) const
{
  if (m_featureId != rhs.m_featureId)
    return m_featureId < rhs.m_featureId;

  if (m_startSegId != rhs.m_startSegId)
    return m_startSegId < rhs.m_startSegId;

  if (m_endSegId != rhs.m_endSegId)
    return m_endSegId < rhs.m_endSegId;

  if (m_forward != rhs.m_forward)
    return m_forward < rhs.m_forward;

  if (m_start != rhs.m_start)
    return m_start < rhs.m_start;

  return m_end < rhs.m_end;
}

void SegmentRange::Clear()
{
  m_featureId = FeatureID();
  m_startSegId = 0;
  m_endSegId = 0;
  m_forward = true;
  m_start = m2::PointD::Zero();
  m_end = m2::PointD::Zero();
}

bool SegmentRange::IsEmpty() const
{
  return !m_featureId.IsValid() && m_startSegId == 0 && m_endSegId == 0 && m_forward &&
         m_start == m2::PointD::Zero() && m_end == m2::PointD::Zero();
}

FeatureID const & SegmentRange::GetFeature() const
{
  return m_featureId;
}

bool SegmentRange::IsCorrect() const
{
  return (m_forward && m_startSegId <= m_endSegId) || (!m_forward && m_endSegId <= m_startSegId);
}

bool SegmentRange::GetFirstSegment(NumMwmIds const & numMwmIds, Segment & segment) const
{
  return GetSegmentBySegId(m_startSegId, numMwmIds, segment);
}

bool SegmentRange::GetLastSegment(NumMwmIds const & numMwmIds, Segment & segment) const
{
  return GetSegmentBySegId(m_endSegId, numMwmIds, segment);
}

bool SegmentRange::GetSegmentBySegId(uint32_t segId, NumMwmIds const & numMwmIds,
                                     Segment & segment) const
{
  if (!m_featureId.IsValid())
    return false;

  segment = Segment(numMwmIds.GetId(platform::CountryFile(m_featureId.GetMwmName())),
                    m_featureId.m_index, segId, m_forward);
  return true;
}

string DebugPrint(SegmentRange const & segmentRange)
{
  stringstream out;
  out << "SegmentRange "
      << "{ m_featureId = " << DebugPrint(segmentRange.m_featureId)
      << ", m_startSegId = " << segmentRange.m_startSegId
      << ", m_endSegId = " << segmentRange.m_endSegId
      << ", m_forward = " << segmentRange.m_forward
      << ", m_start = " << DebugPrint(segmentRange.m_start)
      << ", m_end = " << DebugPrint(segmentRange.m_end)
      << " }";
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
  out << "TurnItem "
      << "{ m_index = " << turnItem.m_index
      << ", m_turn = " << DebugPrint(turnItem.m_turn)
      << ", m_lanes = " << ::DebugPrint(turnItem.m_lanes)
      << ", m_exitNum = " << turnItem.m_exitNum
      << ", m_pedestrianDir = " << ::DebugPrint(turnItem.m_pedestrianTurn)
      << " }";
  return out.str();
}

string DebugPrint(TurnItemDist const & turnItemDist)
{
  stringstream out;
  out << "TurnItemDist "
      << "{ m_turnItem = " << DebugPrint(turnItemDist.m_turnItem)
      << ", m_distMeters = " << turnItemDist.m_distMeters
      << " }";
  return out.str();
}

string GetTurnString(CarDirection turn)
{
  std::string turnStr = ::DebugPrint(turn);
  if (!turnStr.empty())
    return turnStr;

  ASSERT(false, (static_cast<int>(turn)));
  return "unknown CarDirection";
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

bool IsTurnMadeFromLeft(CarDirection t)
{
  return IsLeftTurn(t) || t == CarDirection::UTurnLeft || t == CarDirection::ExitHighwayToLeft;
}

bool IsTurnMadeFromRight(CarDirection t)
{
  return IsRightTurn(t) || t == CarDirection::UTurnRight || t == CarDirection::ExitHighwayToRight;
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
      return l == LaneWay::through;
    case CarDirection::TurnRight:
      return l == LaneWay::right;
    case CarDirection::TurnSharpRight:
      return l == LaneWay::sharp_right;
    case CarDirection::TurnSlightRight:
    case CarDirection::ExitHighwayToRight:
      return l == LaneWay::slight_left;
    case CarDirection::TurnLeft:
      return l == LaneWay::left;
    case CarDirection::TurnSharpLeft:
      return l == LaneWay::sharp_left;
    case CarDirection::TurnSlightLeft:
    case CarDirection::ExitHighwayToLeft:
      return l == LaneWay::slight_left;
    case CarDirection::UTurnLeft:
    case CarDirection::UTurnRight:
      return l == LaneWay::reverse;
  }
}

bool IsLaneWayConformedTurnDirectionApproximately(LaneWay l, CarDirection t)
{
  switch (t)
  {
    default:
      return false;
    case CarDirection::GoStraight:
      return l == LaneWay::through || l == LaneWay::slight_right || l == LaneWay::slight_left;
    case CarDirection::TurnRight:
      return l == LaneWay::right || l == LaneWay::sharp_right || l == LaneWay::slight_right;
    case CarDirection::TurnSharpRight:
      return l == LaneWay::sharp_right || l == LaneWay::right;
    case CarDirection::TurnSlightRight:
      return l == LaneWay::slight_right || l == LaneWay::through || l == LaneWay::right;
    case CarDirection::TurnLeft:
      return l == LaneWay::left || l == LaneWay::slight_left || l == LaneWay::sharp_left;
    case CarDirection::TurnSharpLeft:
      return l == LaneWay::sharp_left || l == LaneWay::left;
    case CarDirection::TurnSlightLeft:
      return l == LaneWay::slight_left || l == LaneWay::through || l == LaneWay::left;
    case CarDirection::UTurnLeft:
    case CarDirection::UTurnRight:
      return l == LaneWay::reverse;
    case CarDirection::ExitHighwayToLeft:
      return l == LaneWay::slight_left || l == LaneWay::left;
    case CarDirection::ExitHighwayToRight:
      return l == LaneWay::slight_right || l == LaneWay::right;
  }
}

bool IsLaneUnrestricted(const SingleLaneInfo & lane)
{
  /// @todo Is there any reason to store None single lane?
  return lane.m_lane.size() == 1 && lane.m_lane[0] == LaneWay::none;
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
  // When `laneString` ends with "" representing none, for example, in "right;",
  // `getline` will not read any characters, so it exits the loop and does not
  // handle the "". So, we add a delimiter to the end of `laneString`. Nonempty
  // final tokens consume the delimiter and act as expected, and empty final tokens
  // read a the delimiter, so `getline` sets `token` to the empty string rather than
  // exiting the loop.
  istringstream laneStream(laneString + delimiter);
  string token;
  while (getline(laneStream, token, delimiter))
  {
    if (auto laneWay = magic_enum::enum_cast<LaneWay>(token); laneWay.has_value())
      lane.push_back(*laneWay);
    else if (token.empty())
      lane.push_back(LaneWay::none);
    else
      return false;
  }
  return true;
}

bool ParseLanes(string lanesString, vector<SingleLaneInfo> & lanes)
{
  if (lanesString.empty())
    return false;
  lanes.clear();
  strings::AsciiToLower(lanesString);
  base::EraseIf(lanesString, [](char c) { return isspace(c); });

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

string DebugPrint(CarDirection const turn)
{
  return GetTurnString(turn);
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
