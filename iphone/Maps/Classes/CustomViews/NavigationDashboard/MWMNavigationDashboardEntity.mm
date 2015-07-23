//
//  MWMNavigationDashboardEntity.m
//  Maps
//
//  Created by v.mikhaylenko on 22.07.15.
//  Copyright (c) 2015 MapsWithMe. All rights reserved.
//

#import "MWMNavigationDashboardEntity.h"
#import "Framework.h"

@implementation MWMNavigationDashboardEntity

- (instancetype)initWithFollowingInfo:(location::FollowingInfo const &)info
{
  self = [super init];
  if (self)
    [self configure:info];
  return self;
}

- (void)updateWithFollowingInfo:(location::FollowingInfo const &)info
{
  [self configure:info];
}

- (void)configure:(location::FollowingInfo const &)info
{
  _timeToTarget = info.m_time;
  _targetDistance = [NSString stringWithUTF8String:info.m_distToTarget.c_str()];
  _targetUnits = [NSString stringWithUTF8String:info.m_targetUnitsSuffix.c_str()];
  _distanceToTurn = [NSString stringWithUTF8String:info.m_distToTurn.c_str()];
  _turnUnits = [NSString stringWithUTF8String:info.m_turnUnitsSuffix.c_str()];
  _turnImage = image(info.m_turn);
  if (info.m_turn == routing::turns::TurnDirection::EnterRoundAbout)
    _roundExitNumber = info.m_exitNum;
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
