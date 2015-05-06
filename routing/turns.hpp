#pragma once

#include "std/initializer_list.hpp"
#include "std/iostream.hpp"
#include "std/string.hpp"
#include "std/vector.hpp"

#include "geometry/point2d.hpp"

namespace routing
{

namespace turns
{
// @todo(vbykoianko) It's a good idea to gather all the turns information into one entity.
// For the time being several separate entities reflect turn information. Like Route::TurnsT or
// turns::TurnsGeomT

// Do not change the order of right and left turns
// TurnRight(TurnLeft) must have a minimal value
// TurnSlightRight(TurnSlightLeft) must have a maximum value
// to make check TurnRight <= turn <= TurnSlightRight work
//
// TurnDirection array in cpp file must be synchronized with state of TurnDirection enum in java.
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

  UTurn,

  TakeTheExit,

  EnterRoundAbout,
  LeaveRoundAbout,
  StayOnRoundAbout,

  StartAtEndOfStreet,
  ReachedYourDestination,
  Count  // This value is used for internals only.
};

string DebugPrint(TurnDirection const l);

// LaneWay array in cpp file must be synchronized with state of LaneWay enum in java.
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
  Count  // This value is used for internals only.
};

string DebugPrint(LaneWay const l);

struct TurnGeom
{
  TurnGeom(uint32_t indexInRoute, uint32_t turnIndex,
           vector<m2::PointD>::const_iterator b, vector<m2::PointD>::const_iterator e) :
    m_indexInRoute(indexInRoute), m_turnIndex(turnIndex), m_points(b, e)
  {
  }

  bool operator==(TurnGeom const & other) const;

  uint32_t m_indexInRoute;
  uint32_t m_turnIndex;
  vector<m2::PointD> m_points;
};

string DebugPrint(TurnGeom const & turnGeom);

typedef vector<turns::TurnGeom> TurnsGeomT;
typedef vector<LaneWay> TSingleLane;

struct SingleLaneInfo
{
  SingleLaneInfo(initializer_list<LaneWay> const & l = {}) : m_lane(l), m_isActive(false) {}

  TSingleLane m_lane;
  bool m_isActive;

  bool operator==(SingleLaneInfo const & other) const;
};

string DebugPrint(SingleLaneInfo const & singleLaneInfo);

string const GetTurnString(TurnDirection turn);

bool IsLeftTurn(TurnDirection t);
bool IsRightTurn(TurnDirection t);
bool IsLeftOrRightTurn(TurnDirection t);
bool IsStayOnRoad(TurnDirection t);
bool IsGoStraightOrSlightTurn(TurnDirection t);

bool IsLaneWayConformedTurnDirection(LaneWay l, TurnDirection t);
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

}
}

