#import "MWMLocationManager.h"
#import "MWMNavigationDashboardEntity.h"
#import "MWMNavigationDashboardManager+Entity.h"
#import "MWMRouter.h"
#import "SwiftBridge.h"

#include "geometry/distance_on_sphere.hpp"
#include "routing/turns.hpp"

namespace
{
UIImage * image(routing::turns::CarDirection t, bool isNextTurn)
{
  if ([MWMRouter type] == MWMRouterTypePedestrian)
    return [UIImage imageNamed:@"ic_direction"];

  using namespace routing::turns;
  NSString * imageName;
  switch (t)
  {
  case CarDirection::TurnSlightRight: imageName = @"slight_right"; break;
  case CarDirection::TurnRight: imageName = @"simple_right"; break;
  case CarDirection::TurnSharpRight: imageName = @"sharp_right"; break;
  case CarDirection::TurnSlightLeft: imageName = @"slight_left"; break;
  case CarDirection::TurnLeft: imageName = @"simple_left"; break;
  case CarDirection::TurnSharpLeft: imageName = @"sharp_left"; break;
  case CarDirection::UTurnLeft: imageName = @"uturn_left"; break;
  case CarDirection::UTurnRight: imageName = @"uturn_right"; break;
  case CarDirection::ReachedYourDestination: imageName = @"finish_point"; break;
  case CarDirection::LeaveRoundAbout:
  case CarDirection::EnterRoundAbout: imageName = @"round"; break;
  case CarDirection::GoStraight: imageName = @"straight"; break;
  case CarDirection::StartAtEndOfStreet:
  case CarDirection::StayOnRoundAbout:
  case CarDirection::TakeTheExit:
  case CarDirection::Count:
  case CarDirection::None: imageName = isNextTurn ? nil : @"straight"; break;
  }
  if (!imageName)
    return nil;
  return [UIImage imageNamed:isNextTurn ? [imageName stringByAppendingString:@"_then"] : imageName];
}
}  // namespace

@interface MWMNavigationDashboardEntity ()

@property(nonatomic, readwrite) CLLocation * pedestrianDirectionPosition;
@property(nonatomic, readwrite) BOOL isValid;
@property(nonatomic, readwrite) NSString * targetDistance;
@property(nonatomic, readwrite) NSString * targetUnits;
@property(nonatomic, readwrite) NSString * distanceToTurn;
@property(nonatomic, readwrite) NSString * turnUnits;
@property(nonatomic, readwrite) NSString * streetName;
@property(nonatomic, readwrite) UIImage * turnImage;
@property(nonatomic, readwrite) UIImage * nextTurnImage;
@property(nonatomic, readwrite) NSUInteger roundExitNumber;
@property(nonatomic, readwrite) NSUInteger timeToTarget;
@property(nonatomic, readwrite) CGFloat progress;
@property(nonatomic, readwrite) BOOL isPedestrian;
@property(nonatomic, readwrite) NSAttributedString * estimate;

@end

@interface MWMNavigationDashboardManager ()

@property(nonatomic) MWMNavigationDashboardEntity * entity;

- (void)onNavigationInfoUpdated;

@end

@implementation MWMNavigationDashboardManager (Entity)

- (void)updateFollowingInfo:(location::FollowingInfo const &)info
{
  if ([MWMRouter isRouteFinished])
  {
    [MWMRouter stopRouting];
    AudioServicesPlaySystemSound(kSystemSoundID_Vibrate);
    return;
  }

  if (auto entity = self.entity)
  {
    entity.isValid = info.IsValid();
    entity.timeToTarget = info.m_time;
    entity.targetDistance = @(info.m_distToTarget.c_str());
    entity.targetUnits = @(info.m_targetUnitsSuffix.c_str());
    entity.progress = info.m_completionPercent;
    CLLocation * lastLocation = [MWMLocationManager lastLocation];
    if (lastLocation && [MWMRouter type] == MWMRouterTypePedestrian)
    {
      entity.isPedestrian = YES;
      string distance;
      CLLocationCoordinate2D const & coordinate = lastLocation.coordinate;

      auto const lat = info.m_pedestrianDirectionPos.lat;
      auto const lon = info.m_pedestrianDirectionPos.lon;
      entity.pedestrianDirectionPosition = [[CLLocation alloc] initWithLatitude:lat longitude:lon];
      // TODO: Not the best solution, but this solution is temporary and will be replaced in future
      measurement_utils::FormatDistance(
          ms::DistanceOnEarth(coordinate.latitude, coordinate.longitude, lat, lon), distance);
      istringstream is(distance);
      string dist;
      string units;
      is >> dist;
      is >> units;
      entity.nextTurnImage = nil;
      entity.distanceToTurn = @(dist.c_str());
      entity.turnUnits = @(units.c_str());
      entity.streetName = @"";
    }
    else
    {
      entity.isPedestrian = NO;
      entity.distanceToTurn = @(info.m_distToTurn.c_str());
      entity.turnUnits = @(info.m_turnUnitsSuffix.c_str());
      entity.streetName = @(info.m_targetName.c_str());
      entity.nextTurnImage = image(info.m_nextTurn, true);
    }

    NSDictionary * etaAttributes = @{
      NSForegroundColorAttributeName : UIColor.blackPrimaryText,
      NSFontAttributeName : UIFont.medium17
    };
    NSString * eta = [NSDateComponentsFormatter etaStringFrom:entity.timeToTarget];
    NSString * resultString =
        [NSString stringWithFormat:@"%@ • %@ %@", eta, entity.targetDistance, entity.targetUnits];
    NSMutableAttributedString * result =
        [[NSMutableAttributedString alloc] initWithString:resultString];
    [result addAttributes:etaAttributes range:NSMakeRange(0, resultString.length)];
    entity.estimate = [result copy];

    using namespace routing::turns;
    CarDirection const turn = info.m_turn;
    entity.turnImage = image(turn, false);
    BOOL const isRound = turn == CarDirection::EnterRoundAbout ||
                         turn == CarDirection::StayOnRoundAbout ||
                         turn == CarDirection::LeaveRoundAbout;
    if (isRound)
      entity.roundExitNumber = info.m_exitNum;
    else
      entity.roundExitNumber = 0;
  }

  [self onNavigationInfoUpdated];
}

@end
