#import "MWMLocationManager.h"
#import "MWMRouterTransitStepInfo.h"
#import "MWMRoutingManager.h"
#import "RouteInfo+Core.h"

#import <CoreApi/DurationFormatter.h>
#import <CoreApi/Framework.h>

#include "routing/following_info.hpp"
#include "routing/turns.hpp"

#include "map/routing_manager.hpp"

#include "platform/location.hpp"

#include "geometry/distance_on_sphere.hpp"

namespace
{
NSString * turnImageName(routing::turns::CarDirection turn, BOOL isCarPlay, BOOL isPrimary)
{
  if (![MWMLocationManager lastLocation])
    return nil;

  using namespace routing::turns;
  NSString * imageName = nil;
  switch (turn)
  {
  case CarDirection::ExitHighwayToRight:
    imageName = isCarPlay ? @"ic_cp_exit_highway_to_right" : @"ic_exit_highway_to_right";
    break;
  case CarDirection::TurnSlightRight: imageName = isCarPlay ? @"ic_cp_slight_right" : @"slight_right"; break;
  case CarDirection::TurnRight: imageName = isCarPlay ? @"ic_cp_simple_right" : @"simple_right"; break;
  case CarDirection::TurnSharpRight: imageName = isCarPlay ? @"ic_cp_sharp_right" : @"sharp_right"; break;
  case CarDirection::ExitHighwayToLeft:
    imageName = isCarPlay ? @"ic_cp_exit_highway_to_left" : @"ic_exit_highway_to_left";
    break;
  case CarDirection::TurnSlightLeft: imageName = isCarPlay ? @"ic_cp_slight_left" : @"slight_left"; break;
  case CarDirection::TurnLeft: imageName = isCarPlay ? @"ic_cp_simple_left" : @"simple_left"; break;
  case CarDirection::TurnSharpLeft: imageName = isCarPlay ? @"ic_cp_sharp_left" : @"sharp_left"; break;
  case CarDirection::UTurnLeft: imageName = isCarPlay ? @"ic_cp_uturn_left" : @"uturn_left"; break;
  case CarDirection::UTurnRight: imageName = isCarPlay ? @"ic_cp_uturn_right" : @"uturn_right"; break;
  case CarDirection::ReachedYourDestination: imageName = isCarPlay ? @"ic_cp_finish_point" : @"finish_point"; break;
  case CarDirection::LeaveRoundAbout:
  case CarDirection::EnterRoundAbout: imageName = isCarPlay ? @"ic_cp_round" : @"round"; break;
  case CarDirection::GoStraight: imageName = isCarPlay ? @"ic_cp_straight" : @"straight"; break;
  case CarDirection::StartAtEndOfStreet:
  case CarDirection::StayOnRoundAbout:
  case CarDirection::Count:
  case CarDirection::None: imageName = isPrimary ? (isCarPlay ? @"ic_cp_straight" : @"straight") : nil; break;
  }
  if (!imageName)
    return nil;
  return isPrimary ? [imageName stringByAppendingString:isCarPlay ? @"%@_then" : @"_then"] : imageName;
}

NSString * pedestrianImageName(routing::turns::PedestrianDirection t)
{
  if (![MWMLocationManager lastLocation])
    return nil;

  using namespace routing::turns;
  NSString * imageName;
  switch (t)
  {
  case PedestrianDirection::TurnRight: imageName = @"simple_right"; break;
  case PedestrianDirection::TurnLeft: imageName = @"simple_left"; break;
  case PedestrianDirection::ReachedYourDestination: imageName = @"finish_point"; break;
  case PedestrianDirection::GoStraight:
  case PedestrianDirection::Count:
  case PedestrianDirection::None: imageName = @"straight"; break;
  }
  if (!imageName)
    return nil;
  return imageName;
}

NSArray<MWMRouterTransitStepInfo *> * buildRouteTransitSteps(NSArray<MWMRoutePoint *> * points)
{
  // Generate step info in format: (Segment 1 distance) (1) (Segment 2 distance) (2) ... (n-1) (Segment N distance).
  NSMutableArray<MWMRouterTransitStepInfo *> * steps = [NSMutableArray arrayWithCapacity:[points count] * 2 - 1];
  auto const numPoints = [points count];
  for (int i = 0; i < numPoints - 1; i++)
  {
    MWMRoutePoint * segmentStart = points[i];
    MWMRoutePoint * segmentEnd = points[i + 1];
    auto const distance = platform::Distance::CreateFormatted(
        ms::DistanceOnEarth(segmentStart.latitude, segmentStart.longitude, segmentEnd.latitude, segmentEnd.longitude));

    MWMRouterTransitStepInfo * segmentInfo = [[MWMRouterTransitStepInfo alloc] init];
    segmentInfo.type = MWMRouterTransitTypeRuler;
    segmentInfo.distance = @(distance.GetDistanceString().c_str());
    segmentInfo.distanceUnits = @(distance.GetUnitsString().c_str());
    steps[i * 2] = segmentInfo;

    if (i < numPoints - 2)
    {
      MWMRouterTransitStepInfo * stopInfo = [[MWMRouterTransitStepInfo alloc] init];
      stopInfo.type = MWMRouterTransitTypeIntermediatePoint;
      stopInfo.intermediateIndex = i;
      steps[i * 2 + 1] = stopInfo;
    }
  }

  return steps;
}

NSUnitLength * unitLengthForUnits(platform::Distance::Units units)
{
  switch (units)
  {
  case platform::Distance::Units::Meters: return [NSUnitLength meters]; break;
  case platform::Distance::Units::Kilometers: return [NSUnitLength kilometers]; break;
  case platform::Distance::Units::Feet: return [NSUnitLength feet]; break;
  case platform::Distance::Units::Miles: return [NSUnitLength miles]; break;
  }
}
}  // namespace

@interface MWMRouterTransitStepInfo ()

- (instancetype)initWithStepInfo:(TransitStepInfo const &)info;

@end

@interface RouteInfo ()

@property(nonatomic, readwrite) BOOL showEta;
@property(nonatomic, readwrite) BOOL isWalk;

@end

@implementation RouteInfo

// MARK: - Initializers

- (instancetype)initWithFollowingInfo:(routing::FollowingInfo const &)info
                          routePoints:(NSArray<MWMRoutePoint *> *)points
                                 type:(MWMRouterType)type
                            isCarPlay:(BOOL)isCarPlay
{
  self = [super init];
  if (self)
  {
    BOOL const showEta = (type != MWMRouterTypeRuler);
    _timeToTarget = info.m_time;
    _targetDistance = info.m_distToTarget.GetDistance();
    _targetDistanceString = @(info.m_distToTarget.GetDistanceString().c_str());
    _targetUnits = unitLengthForUnits(info.m_distToTarget.GetUnits());
    _targetUnitsString = @(info.m_distToTarget.GetUnitsString().c_str());
    _distanceToTurn = info.m_distToTurn.GetDistance();
    _distanceToTurnString = @(info.m_distToTurn.GetDistanceString().c_str());
    _progress = info.m_completionPercent;
    _turnUnits = unitLengthForUnits(info.m_distToTurn.GetUnits());
    _turnUnitsString = @(info.m_distToTurn.GetUnitsString().c_str());
    _streetName = @(info.m_nextStreetName.c_str());
    _speedLimitMps = info.m_speedLimitMps;
    if (auto const location = [MWMLocationManager lastLocation])
    {
      if (location.speed > 0)
        _currentSpeedMps = location.speed;
    }

    _isWalk = NO;
    _showEta = showEta;

    if (type == MWMRouterTypeRuler && [points count] > 2)
      _transitSteps = buildRouteTransitSteps(points);
    else
      _transitSteps = [[NSArray alloc] init];

    if (type == MWMRouterTypePedestrian)
      _turnImageName = pedestrianImageName(info.m_pedestrianTurn);
    else
    {
      using namespace routing::turns;
      CarDirection const turn = info.m_turn;
      _turnImageName = turnImageName(turn, NO, NO);
      _nextTurnImageName = turnImageName(info.m_nextTurn, NO, YES);
      BOOL const isRound = turn == CarDirection::EnterRoundAbout || turn == CarDirection::StayOnRoundAbout ||
                           turn == CarDirection::LeaveRoundAbout;
      if (isRound)
        _roundExitNumber = info.m_exitNum;
    }
  }
  return self;
}

- (instancetype)initWithTransitInfo:(TransitRouteInfo const &)info
{
  self = [super init];
  if (self)
  {
    _timeToTarget = info.m_totalTimeInSec;
    _targetDistanceString = @(info.m_totalPedestrianDistanceStr.c_str());
    _targetUnitsString = @(info.m_totalPedestrianUnitsSuffix.c_str());
    _isWalk = YES;
    _showEta = YES;
    NSMutableArray<MWMRouterTransitStepInfo *> * transitSteps = [NSMutableArray new];
    for (auto const & stepInfo : info.m_steps)
      [transitSteps addObject:[[MWMRouterTransitStepInfo alloc] initWithStepInfo:stepInfo]];
    _transitSteps = transitSteps;
  }
  return self;
}

// MARK: - Public methods

- (NSString *)arrival
{
  auto arrivalDate = [[NSDate date] dateByAddingTimeInterval:self.timeToTarget];
  return [DateTimeFormatter dateStringFrom:arrivalDate
                                 dateStyle:NSDateFormatterNoStyle
                                 timeStyle:NSDateFormatterShortStyle];
}

+ (NSAttributedString *)estimateDot
{
  auto attributes =
      @{NSForegroundColorAttributeName: [UIColor blackSecondaryText], NSFontAttributeName: [UIFont medium17]};
  return [[NSAttributedString alloc] initWithString:@" • " attributes:attributes];
}

- (NSAttributedString *)estimate
{
  NSDictionary * primaryAttributes =
      @{NSForegroundColorAttributeName: [UIColor blackPrimaryText], NSFontAttributeName: [UIFont semibold17]};
  NSDictionary * secondaryAttributes =
      @{NSForegroundColorAttributeName: [UIColor blackSecondaryText], NSFontAttributeName: [UIFont medium17]};

  auto result = [[NSMutableAttributedString alloc] initWithString:@""];
  if (self.showEta)
  {
    NSString * eta = [DurationFormatter durationStringFromTimeInterval:self.timeToTarget];
    [result appendAttributedString:[[NSMutableAttributedString alloc] initWithString:eta attributes:primaryAttributes]];
    [result appendAttributedString:RouteInfo.estimateDot];
  }

  if (self.isWalk)
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
    [attrStringWithImage addAttributes:secondaryAttributes range:NSMakeRange(0, attrStringWithImage.length)];
    [result appendAttributedString:attrStringWithImage];
  }

  if (self.targetDistance && self.targetUnits)
  {
    auto target = [NSString stringWithFormat:@"%@ %@", self.targetDistanceString, self.targetUnitsString];
    [result appendAttributedString:[[NSAttributedString alloc] initWithString:target attributes:secondaryAttributes]];
  }

  return result;
}

@end
