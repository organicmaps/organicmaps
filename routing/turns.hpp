#pragma once

#include "geometry/point2d.hpp"

#include "3party/osrm/osrm-backend/typedefs.h"

#include "std/initializer_list.hpp"
#include "std/limits.hpp"
#include "std/string.hpp"
#include "std/vector.hpp"

namespace routing
{
using TNodeId = uint32_t;
using TEdgeWeight = double;

namespace turns
{
/// @todo(vbykoianko) It's a good idea to gather all the turns information into one entity.
/// For the time being several separate entities reflect the turn information. Like Route::TTurns

using TGeomTurnCandidate = vector<double>;

double constexpr kFeaturesNearTurnMeters = 3.0;

/*!
 * \warning The order of values below shall not be changed.
 * TurnRight(TurnLeft) must have a minimal value and
 * TurnSlightRight(TurnSlightLeft) must have a maximum value
 * \warning The values of TurnDirection shall be synchronized with values of TurnDirection enum in
 * java.
 */
enum class TurnDirection
{
  NoTurn = 0,
  GoStraight,

  TurnRight,
  TurnSharpRight,
  TurnSlightRight,

  TurnLeft,
  TurnSharpLeft,
  TurnSlightLeft,

  UTurnLeft,
  UTurnRight,

  TakeTheExit,

  EnterRoundAbout,
  LeaveRoundAbout,
  StayOnRoundAbout,

  StartAtEndOfStreet,
  ReachedYourDestination,
  Count  /**< This value is used for internals only. */
};

string DebugPrint(TurnDirection const l);

/*!
 * \warning The values of PedestrianDirectionType shall be synchronized with values in java
 */
enum class PedestrianDirection
{
  None = 0,
  Upstairs,
  Downstairs,
  LiftGate,
  Gate,
  ReachedYourDestination,
  Count  /**< This value is used for internals only. */
};

string DebugPrint(PedestrianDirection const l);

/*!
 * \warning The values of LaneWay shall be synchronized with values of LaneWay enum in java.
 */
enum class LaneWay
{
  None = 0,
  Reverse,
  SharpLeft,
  Left,
  SlightLeft,
  MergeToRight,
  Through,
  MergeToLeft,
  SlightRight,
  Right,
  SharpRight,
  Count  /**< This value is used for internals only. */
};

string DebugPrint(LaneWay const l);

typedef vector<LaneWay> TSingleLane;

struct SingleLaneInfo
{
  TSingleLane m_lane;
  bool m_isRecommended = false;

  SingleLaneInfo() = default;
  SingleLaneInfo(initializer_list<LaneWay> const & l) : m_lane(l) {}
  bool operator==(SingleLaneInfo const & other) const;
};

string DebugPrint(SingleLaneInfo const & singleLaneInfo);

struct TurnItem
{
  TurnItem()
      : m_index(numeric_limits<uint32_t>::max()),
        m_turn(TurnDirection::NoTurn),
        m_exitNum(0),
        m_keepAnyway(false),
        m_pedestrianTurn(PedestrianDirection::None)
  {
  }

  TurnItem(uint32_t idx, TurnDirection t, uint32_t exitNum = 0)
      : m_index(idx), m_turn(t), m_exitNum(exitNum), m_keepAnyway(false)
      , m_pedestrianTurn(PedestrianDirection::None)
  {
  }

  TurnItem(uint32_t idx, PedestrianDirection p)
      : m_index(idx), m_turn(TurnDirection::NoTurn), m_exitNum(0), m_keepAnyway(false)
      , m_pedestrianTurn(p)
  {
  }

  bool operator==(TurnItem const & rhs) const
  {
    return m_index == rhs.m_index && m_turn == rhs.m_turn && m_lanes == rhs.m_lanes &&
           m_exitNum == rhs.m_exitNum && m_sourceName == rhs.m_sourceName &&
           m_targetName == rhs.m_targetName && m_keepAnyway == rhs.m_keepAnyway &&
           m_pedestrianTurn == rhs.m_pedestrianTurn;
  }

  uint32_t m_index;               /*!< Index of point on polyline (number of segment + 1). */
  TurnDirection m_turn;           /*!< The turn instruction of the TurnItem */
  vector<SingleLaneInfo> m_lanes; /*!< Lane information on the edge before the turn. */
  uint32_t m_exitNum;             /*!< Number of exit on roundabout. */
  string m_sourceName;            /*!< Name of the street which the ingoing edge belongs to */
  string m_targetName;            /*!< Name of the street which the outgoing edge belongs to */
  /*!
   * \brief m_keepAnyway is true if the turn shall not be deleted
   * and shall be demonstrated to an end user.
   */
  bool m_keepAnyway;
  /*!
   * \brief m_pedestrianTurn is type of corresponding direction for a pedestrian, or None
   * if there is no pedestrian specific direction
   */
  PedestrianDirection m_pedestrianTurn;
};

string DebugPrint(TurnItem const & turnItem);

struct TurnItemDist
{
  TurnItem m_turnItem;
  double m_distMeters;
};

string DebugPrint(TurnItemDist const & turnItemDist);

string const GetTurnString(TurnDirection turn);

bool IsLeftTurn(TurnDirection t);
bool IsRightTurn(TurnDirection t);
bool IsLeftOrRightTurn(TurnDirection t);
bool IsStayOnRoad(TurnDirection t);
bool IsGoStraightOrSlightTurn(TurnDirection t);

/*!
 * \param l A variant of going along a lane.
 * \param t A turn direction.
 * \return True if @l corresponds with @t exactly. For example it returns true
 * when @l equals to LaneWay::Right and @t equals to TurnDirection::TurnRight.
 * Otherwise it returns false.
 */
bool IsLaneWayConformedTurnDirection(LaneWay l, TurnDirection t);

/*!
 * \param l A variant of going along a lane.
 * \param t A turn direction.
 * \return True if @l corresponds with @t approximately. For example it returns true
 * when @l equals to LaneWay::Right and @t equals to TurnDirection::TurnSlightRight.
 * Otherwise it returns false.
 */
bool IsLaneWayConformedTurnDirectionApproximately(LaneWay l, TurnDirection t);

/*!
 * \brief Parse lane information which comes from @lanesString
 * \param lanesString lane information. Example through|through|through|through;right
 * \param lanes the result of parsing.
 * \return true if @lanesString parsed successfully, false otherwise.
 * Note 1: if @lanesString is empty returns false.
 * Note 2: @laneString is passed by value on purpose. It'll be used(changed) in the method.
 */
bool ParseLanes(string lanesString, vector<SingleLaneInfo> & lanes);
void SplitLanes(string const & lanesString, char delimiter, vector<string> & lanes);
bool ParseSingleLane(string const & laneString, char delimiter, TSingleLane & lane);
double PiMinusTwoVectorsAngle(m2::PointD const & p, m2::PointD const & p1, m2::PointD const & p2);
}  // namespace turns
}  // namespace routing
