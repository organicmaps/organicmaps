#pragma once

#include "std/string.hpp"
#include "std/vector.hpp"

#include "geometry/point2d.hpp"

namespace routing
{

namespace turns
{

// Do not change the order of right and left turns
// TurnRight(TurnLeft) must have a minimal value
// TurnSlightRight(TurnSlightLeft) must have a maximum value
// to make check TurnRight <= turn <= TurnSlightRight work
//
// turnStrings array in cpp file must be synchronized with state of TurnDirection enum.
enum TurnDirection
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

};

struct TurnGeom
{
  TurnGeom(uint32_t indexInRoute, uint32_t turnIndex,
           vector<m2::PointD>::const_iterator b, vector<m2::PointD>::const_iterator e) :
    m_indexInRoute(indexInRoute), m_turnIndex(turnIndex), m_points(b, e)
  {
  }

  uint32_t m_indexInRoute;
  uint32_t m_turnIndex;
  vector<m2::PointD> m_points;
};

typedef vector<turns::TurnGeom> TurnsGeomT;

string const & GetTurnString(TurnDirection turn);

bool IsLeftTurn(TurnDirection t);
bool IsRightTurn(TurnDirection t);
bool IsLeftOrRightTurn(TurnDirection t);
bool IsStayOnRoad(TurnDirection t);
bool IsGoStraightOrSlightTurn(TurnDirection t);

}
}
