#import "Common.h"
#import "LocationManager.h"
#import "MapsAppDelegate.h"
#import "MWMNavigationDashboardEntity.h"

#include "Framework.h"
#include "geometry/distance_on_sphere.hpp"
#include "platform/measurement_utils.hpp"

using namespace routing::turns;

@implementation MWMNavigationDashboardEntity

- (void)updateWithFollowingInfo:(location::FollowingInfo const &)info
{
  [self configure:info];
}

- (void)configure:(location::FollowingInfo const &)info
{
  _timeToTarget = info.m_time;
  _targetDistance = @(info.m_distToTarget.c_str());
  _targetUnits = @(info.m_targetUnitsSuffix.c_str());
  _progress = info.m_completionPercent;
  auto & f = GetFramework();
  if (f.GetRouter() == routing::RouterType::Pedestrian)
  {
    _isPedestrian = YES;
    string distance;
    CLLocationCoordinate2D const & coordinate ([MapsAppDelegate theApp].m_locationManager.lastLocation.coordinate);
    ms::LatLon const & directionPos = info.m_pedestrianDirectionPos;
    //TODO: Not the best solution, but this solution is temporary and will be replaced in future
    MeasurementUtils::FormatDistance(ms::DistanceOnEarth(coordinate.latitude, coordinate.longitude,
                                          directionPos.lat, directionPos.lon), distance);
    istringstream is (distance);
    string dist;
    string units;
    is>>dist;
    is>>units;
    _nextTurnImage = nil;
    _distanceToTurn = @(dist.c_str());
    _turnUnits = @(units.c_str());
    _streetName = @"";
//    _lanes = {};
  }
  else
  {
    _isPedestrian = NO;
    _distanceToTurn = @(info.m_distToTurn.c_str());
    _turnUnits = @(info.m_turnUnitsSuffix.c_str());
    _streetName = @(info.m_targetName.c_str());
//    _lanes = info.m_lanes;
    _nextTurnImage = image(info.m_nextTurn, true);
  }

  TurnDirection const turn = info.m_turn;
  _turnImage = image(turn, false);
  BOOL const isRound = turn == TurnDirection::EnterRoundAbout ||
                       turn == TurnDirection::StayOnRoundAbout ||
                       turn == TurnDirection::LeaveRoundAbout;
  if (isRound)
    _roundExitNumber = info.m_exitNum;
  else
    _roundExitNumber = 0;
}

UIImage * image(routing::turns::TurnDirection t, bool isNextTurn)
{
  if (GetFramework().GetRouter() == routing::RouterType::Pedestrian)
    return [UIImage imageNamed:@"ic_direction"];

  NSString * imageName;
  switch (t)
  {
    case TurnDirection::TurnSlightRight:
      imageName = @"slight_right";
      break;
    case TurnDirection::TurnRight:
      imageName = @"simple_right";
      break;
    case TurnDirection::TurnSharpRight:
      imageName = @"sharp_right";
      break;
    case TurnDirection::TurnSlightLeft:
      imageName = @"slight_left";
      break;
    case TurnDirection::TurnLeft:
      imageName = @"simple_left";
      break;
    case TurnDirection::TurnSharpLeft:
      imageName = @"sharp_left";
      break;
    case TurnDirection::UTurnLeft:
      imageName = @"uturn_left";
      break;
    case TurnDirection::UTurnRight:
      imageName = @"uturn_right";
      break;
    case TurnDirection::ReachedYourDestination:
      imageName = @"finish_point";
      break;
    case TurnDirection::LeaveRoundAbout:
    case TurnDirection::EnterRoundAbout:
      imageName = @"round";
      break;
    case TurnDirection::GoStraight:
      imageName = @"straight";
      break;
    case TurnDirection::StartAtEndOfStreet:
    case TurnDirection::StayOnRoundAbout:
    case TurnDirection::TakeTheExit:
    case TurnDirection::Count:
    case TurnDirection::NoTurn:
      imageName = isNextTurn ? nil : @"straight";
      break;
  }
  if (!imageName)
    return nil;
  return [UIImage imageNamed:isNextTurn ? [imageName stringByAppendingString:@"_then"] : imageName];
}

@end
