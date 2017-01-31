#import "MWMNavigationDashboardEntity.h"
#import "MWMCommon.h"
#import "MWMCoreUnits.h"
#import "MWMLocationManager.h"
#import "MWMSettings.h"
#import "MapsAppDelegate.h"
#import "SwiftBridge.h"

#include "Framework.h"
#include "geometry/distance_on_sphere.hpp"
#include "platform/measurement_utils.hpp"

using namespace routing::turns;

@implementation MWMNavigationDashboardEntity

- (void)updateFollowingInfo:(location::FollowingInfo const &)info
{
  _isValid = info.IsValid();
  _timeToTarget = info.m_time;
  _targetDistance = @(info.m_distToTarget.c_str());
  _targetUnits = @(info.m_targetUnitsSuffix.c_str());
  _progress = info.m_completionPercent;
  auto & f = GetFramework();
  CLLocation * lastLocation = [MWMLocationManager lastLocation];
  if (lastLocation && f.GetRouter() == routing::RouterType::Pedestrian)
  {
    _isPedestrian = YES;
    string distance;
    CLLocationCoordinate2D const & coordinate = lastLocation.coordinate;
    _pedestrianDirectionPosition = info.m_pedestrianDirectionPos;
    // TODO: Not the best solution, but this solution is temporary and will be replaced in future
    measurement_utils::FormatDistance(
        ms::DistanceOnEarth(coordinate.latitude, coordinate.longitude,
                            _pedestrianDirectionPosition.lat, _pedestrianDirectionPosition.lon),
        distance);
    istringstream is(distance);
    string dist;
    string units;
    is >> dist;
    is >> units;
    _nextTurnImage = nil;
    _distanceToTurn = @(dist.c_str());
    _turnUnits = @(units.c_str());
    _streetName = @"";
  }
  else
  {
    _isPedestrian = NO;
    _distanceToTurn = @(info.m_distToTurn.c_str());
    _turnUnits = @(info.m_turnUnitsSuffix.c_str());
    _streetName = @(info.m_targetName.c_str());
    _nextTurnImage = image(info.m_nextTurn, true);
  }

  NSDictionary * etaAttributes = @{
    NSForegroundColorAttributeName : UIColor.blackPrimaryText,
    NSFontAttributeName : UIFont.medium17
  };
  NSString * eta = [NSDateComponentsFormatter etaStringFrom:_timeToTarget];
  NSString * resultString =
      [NSString stringWithFormat:@"%@ • %@ %@", eta, _targetDistance, _targetUnits];
  NSMutableAttributedString * result =
      [[NSMutableAttributedString alloc] initWithString:resultString];
  [result addAttributes:etaAttributes range:NSMakeRange(0, resultString.length)];
  _estimate = [result copy];

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
  case TurnDirection::TurnSlightRight: imageName = @"slight_right"; break;
  case TurnDirection::TurnRight: imageName = @"simple_right"; break;
  case TurnDirection::TurnSharpRight: imageName = @"sharp_right"; break;
  case TurnDirection::TurnSlightLeft: imageName = @"slight_left"; break;
  case TurnDirection::TurnLeft: imageName = @"simple_left"; break;
  case TurnDirection::TurnSharpLeft: imageName = @"sharp_left"; break;
  case TurnDirection::UTurnLeft: imageName = @"uturn_left"; break;
  case TurnDirection::UTurnRight: imageName = @"uturn_right"; break;
  case TurnDirection::ReachedYourDestination: imageName = @"finish_point"; break;
  case TurnDirection::LeaveRoundAbout:
  case TurnDirection::EnterRoundAbout: imageName = @"round"; break;
  case TurnDirection::GoStraight: imageName = @"straight"; break;
  case TurnDirection::StartAtEndOfStreet:
  case TurnDirection::StayOnRoundAbout:
  case TurnDirection::TakeTheExit:
  case TurnDirection::Count:
  case TurnDirection::NoTurn: imageName = isNextTurn ? nil : @"straight"; break;
  }
  if (!imageName)
    return nil;
  return [UIImage imageNamed:isNextTurn ? [imageName stringByAppendingString:@"_then"] : imageName];
}

- (NSString *)speed
{
  CLLocation * lastLocation = [MWMLocationManager lastLocation];
  if (!lastLocation || lastLocation.speed < 0)
    return nil;
  auto const units = coreUnits([MWMSettings measurementUnits]);
  return @(measurement_utils::FormatSpeed(lastLocation.speed, units).c_str());
}

- (NSString *)speedUnits
{
  auto const units = coreUnits([MWMSettings measurementUnits]);
  return @(measurement_utils::FormatSpeedUnits(units).c_str());
}

@end
