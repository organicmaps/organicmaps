#import "MWMLocationManager.h"
#import "MWMNavigationDashboardEntity.h"
#import "MWMNavigationDashboardManager+Entity.h"
#import "MWMRouter.h"
#import "MWMRouterTransitStepInfo.h"
#import "SwiftBridge.h"

#include "routing/turns.hpp"

#include "map/routing_manager.hpp"

#include "platform/location.hpp"

#include "geometry/distance_on_sphere.hpp"

namespace
{
UIImage * image(routing::turns::CarDirection t, bool isNextTurn)
{
  if (![MWMLocationManager lastLocation])
    return nil;
  if ([MWMRouter type] == MWMRouterTypePedestrian)
    return [UIImage imageNamed:@"ic_direction"];

  using namespace routing::turns;
  NSString * imageName;
  switch (t)
  {
  case CarDirection::ExitHighwayToRight:
  case CarDirection::TurnSlightRight: imageName = @"slight_right"; break;
  case CarDirection::TurnRight: imageName = @"simple_right"; break;
  case CarDirection::TurnSharpRight: imageName = @"sharp_right"; break;
  case CarDirection::ExitHighwayToLeft:
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
  case CarDirection::Count:
  case CarDirection::None: imageName = isNextTurn ? nil : @"straight"; break;
  }
  if (!imageName)
    return nil;
  return [UIImage imageNamed:isNextTurn ? [imageName stringByAppendingString:@"_then"] : imageName];
}

NSAttributedString * estimate(NSTimeInterval time, NSAttributedString * dot, NSString * distance,
                              NSString * distanceUnits, NSDictionary * primaryAttributes,
                              NSDictionary * secondaryAttributes, BOOL isWalk)
{
  NSString * eta = [NSDateComponentsFormatter etaStringFrom:time];
  auto result = [[NSMutableAttributedString alloc] initWithString:eta attributes:primaryAttributes];
  [result appendAttributedString:dot];

  if (isWalk)
  {
    UIFont * font = primaryAttributes[NSFontAttributeName];
    auto textAttachment = [[NSTextAttachment alloc] init];
    auto image = [UIImage imageNamed:@"ic_walk"];
    textAttachment.image = image;
    auto const height = font.lineHeight;
    auto const y = height - image.size.height;
    auto const width = image.size.width * height / image.size.height;
    textAttachment.bounds = CGRectIntegral({{0, y}, {width, height}});

    NSMutableAttributedString * attrStringWithImage =
        [NSAttributedString attributedStringWithAttachment:textAttachment].mutableCopy;
    [attrStringWithImage addAttributes:secondaryAttributes
                                 range:NSMakeRange(0, attrStringWithImage.length)];
    [result appendAttributedString:attrStringWithImage];
  }

  auto target = [NSString stringWithFormat:@"%@ %@", distance, distanceUnits];
  [result appendAttributedString:[[NSAttributedString alloc] initWithString:target
                                                                 attributes:secondaryAttributes]];

  return result;
}
}  // namespace

@interface MWMNavigationDashboardEntity ()

@property(copy, nonatomic, readwrite) NSArray<MWMRouterTransitStepInfo *> * transitSteps;
@property(copy, nonatomic, readwrite) NSAttributedString * estimate;
@property(copy, nonatomic, readwrite) NSString * distanceToTurn;
@property(copy, nonatomic, readwrite) NSString * streetName;
@property(copy, nonatomic, readwrite) NSString * targetDistance;
@property(copy, nonatomic, readwrite) NSString * targetUnits;
@property(copy, nonatomic, readwrite) NSString * turnUnits;
@property(nonatomic, readwrite) BOOL isValid;
@property(nonatomic, readwrite) CGFloat progress;
@property(nonatomic, readwrite) CLLocation * pedestrianDirectionPosition;
@property(nonatomic, readwrite) NSUInteger roundExitNumber;
@property(nonatomic, readwrite) NSUInteger timeToTarget;
@property(nonatomic, readwrite) UIImage * nextTurnImage;
@property(nonatomic, readwrite) UIImage * turnImage;

@end

@interface MWMRouterTransitStepInfo ()

- (instancetype)initWithStepInfo:(TransitStepInfo const &)info;

@end

@interface MWMNavigationDashboardManager ()

@property(copy, nonatomic) NSDictionary * etaAttributes;
@property(copy, nonatomic) NSDictionary * etaSecondaryAttributes;
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
    entity.isValid = YES;
    entity.timeToTarget = info.m_time;
    entity.targetDistance = @(info.m_distToTarget.c_str());
    entity.targetUnits = @(info.m_targetUnitsSuffix.c_str());
    entity.progress = info.m_completionPercent;
    entity.distanceToTurn = @(info.m_distToTurn.c_str());
    entity.turnUnits = @(info.m_turnUnitsSuffix.c_str());
    entity.streetName = @(info.m_displayedStreetName.c_str());
    entity.nextTurnImage = image(info.m_nextTurn, true);

    entity.estimate =
        estimate(entity.timeToTarget, entity.estimateDot, entity.targetDistance, entity.targetUnits,
                 self.etaAttributes, self.etaSecondaryAttributes, NO);

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

- (void)updateTransitInfo:(TransitRouteInfo const &)info
{
  if (auto entity = self.entity)
  {
    entity.isValid = YES;
    entity.estimate = estimate(info.m_totalTimeInSec, entity.estimateDot,
                               @(info.m_totalPedestrianDistanceStr.c_str()),
                               @(info.m_totalPedestrianUnitsSuffix.c_str()), self.etaAttributes,
                               self.etaSecondaryAttributes, YES);
    NSMutableArray<MWMRouterTransitStepInfo *> * transitSteps = [@[] mutableCopy];
    for (auto const & stepInfo : info.m_steps)
      [transitSteps addObject:[[MWMRouterTransitStepInfo alloc] initWithStepInfo:stepInfo]];
    entity.transitSteps = transitSteps;
  }
  [self onNavigationInfoUpdated];
}

@end
