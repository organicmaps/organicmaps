//
//  MWMNavigationDashboardEntity.m
//  Maps
//
//  Created by v.mikhaylenko on 22.07.15.
//  Copyright (c) 2015 MapsWithMe. All rights reserved.
//

#import "LocationManager.h"
#import "MapsAppDelegate.h"
#import "MWMNavigationDashboardEntity.h"

#include "Framework.h"
#include "geometry/distance_on_sphere.hpp"
#include "platform/measurement_utils.hpp"

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
    _distanceToTurn = @(dist.c_str());
    _turnUnits = @(units.c_str());
  }
  else
  {
    _isPedestrian = NO;
    _distanceToTurn = @(info.m_distToTurn.c_str());
    _turnUnits = @(info.m_turnUnitsSuffix.c_str());
  }
  _turnImage = image(info.m_turn);
  if (info.m_turn == routing::turns::TurnDirection::EnterRoundAbout)
    _roundExitNumber = info.m_exitNum;
  else
    _roundExitNumber = 0;
}

UIImage * image(routing::turns::TurnDirection t)
{
  if (GetFramework().GetRouter() == routing::RouterType::Pedestrian)
    return [UIImage imageNamed:@"ic_direction"];

  using namespace routing::turns;
  switch (t)
  {
    case TurnDirection::TurnSlightRight:
      return [UIImage imageNamed:@"slight_right"];
    case TurnDirection::TurnRight:
      return [UIImage imageNamed:@"simple_right"];
    case TurnDirection::TurnSharpRight:
      return [UIImage imageNamed:@"sharp_right"];
    case TurnDirection::TurnSlightLeft:
      return [UIImage imageNamed:@"slight_left"];
    case TurnDirection::TurnLeft:
      return [UIImage imageNamed:@"simple_left"];
    case TurnDirection::TurnSharpLeft:
      return [UIImage imageNamed:@"sharp_left"];
    case TurnDirection::UTurn:
      return [UIImage imageNamed:@"uturn"];
    case TurnDirection::ReachedYourDestination:
      return [UIImage imageNamed:@"finish_point"];
    case TurnDirection::EnterRoundAbout:
      return [UIImage imageNamed:@"round"];
    case TurnDirection::StartAtEndOfStreet:
    case TurnDirection::LeaveRoundAbout:
    case TurnDirection::StayOnRoundAbout:
    case TurnDirection::TakeTheExit:
    case TurnDirection::Count:
    case TurnDirection::GoStraight:
    case TurnDirection::NoTurn:
      return [UIImage imageNamed:@"straight"];
  }}

@end
