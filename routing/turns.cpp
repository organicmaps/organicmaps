#include "turns.hpp"


namespace routing
{

namespace turns
{


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

}

}
