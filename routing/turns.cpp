#include "routing/turns.hpp"


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

bool IsGoStraightOrSlightTurn(TurnDirection t)
{
  return (t == turns::GoStraight || t == turns::TurnSlightLeft || t == turns::TurnSlightRight);
}


void ParseLanesToStrings(string const & lanesString, char delimiter, vector<string> & lanes)
{
  lanes.clear();
  istringstream lanesStream(lanesString);
  string token;
  while (getline(lanesStream, token, delimiter))
  {
    lanes.push_back(token);
  }
}

bool ParseOneLane(string const & laneString, char delimiter, vector<Lane> & lane)
{
  lane.clear();
  istringstream laneStream(laneString);
  string token;
  while (getline(laneStream, token, delimiter))
  {
    Lane l = Lane::NONE;
    // Staring compare with the most offen tokens according to taginfo.openstreetmap.org to minimize number of comparations.
    if (token == "through")
      l = Lane::THROUGH;
    else if (token == "left")
      l = Lane::LEFT;
    else if (token == "right")
      l = Lane::RIGHT;
    else if (token == "none")
      l = Lane::NONE;
    else if (token == "sharp_left")
      l = Lane::SHARP_LEFT;
    else if (token == "slight_left")
      l = Lane::SLIGH_LEFT;
    else if (token == "merge_to_right")
      l = Lane::MERGE_TO_RIGHT;
    else if (token == "merge_to_left")
      l = Lane::MERGE_TO_LEFT;
    else if (token == "slight_right")
      l = Lane::SLIGHT_RIGHT;
    else if (token == "sharp_right")
      l = Lane::SHARP_RIGHT;
    else if (token == "reverse")
      l = Lane::REVERSE;
    else
    {
      lane.clear();
      return false;
    }
    lane.push_back(l);
  }
  return true;
}

bool ParseLanes(string const & lanesString, vector<vector<Lane>> & lanes)
{
  lanes.clear();
  if (lanesString.empty())
    return false;
  // convert lanesString to lower case
  string lanesStringLower;
  lanesStringLower.reserve(lanesString.size());
  transform(lanesString.begin(), lanesString.end(), back_inserter(lanesStringLower), tolower);
  // removing all spaces
  lanesStringLower.erase(remove_if(lanesStringLower.begin(), lanesStringLower.end(), isspace), lanesStringLower.end());

  vector<string> lanesStrings;
  vector<Lane> lane;
  ParseLanesToStrings(lanesStringLower, '|', lanesStrings);
  for (string const & s : lanesStrings)
  {
    if (!ParseOneLane(s, ';', lane))
    {
      lanes.clear();
      return false;
    }
    lanes.push_back(lane);
  }
  return true;
}

}
}
