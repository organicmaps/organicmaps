#pragma once

#include "../std/string.hpp"

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

string const & GetTurnString(TurnDirection turn);

bool IsLeftTurn(TurnDirection t);
bool IsRightTurn(TurnDirection t);
bool IsLeftOrRightTurn(TurnDirection t);
bool IsStayOnRoad(TurnDirection t);

}
}
