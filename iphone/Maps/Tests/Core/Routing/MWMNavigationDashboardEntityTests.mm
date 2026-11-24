#import <XCTest/XCTest.h>

#import "MWMNavigationDashboardEntity.h"
#import "MWMRouterType.h"

#include "map/transit/transit_display.hpp"
#include "routing/following_info.hpp"

#include <limits>

@class MWMRoutePoint;

@interface MWMNavigationDashboardEntity (Testing)
- (instancetype)initWithFollowingInfo:(routing::FollowingInfo const &)info
                          routePoints:(NSArray<MWMRoutePoint *> *)points
                                 type:(MWMRouterType)type;
- (instancetype)initWithTransitInfo:(TransitRouteInfo const &)info;
@end

namespace
{
int const kRawDurations[] = {
    0, 29, 59, 89, 90, 269, 270, 1665, 2085, 3569, 3570, 3630, 86400, 94485, std::numeric_limits<int>::max()};
}

@interface MWMNavigationDashboardEntityTests : XCTestCase
@end

@implementation MWMNavigationDashboardEntityTests

- (void)testFollowingEstimateKeepsRawSeconds
{
  for (int seconds : kRawDurations)
  {
    routing::FollowingInfo info;
    info.m_time = seconds;
    auto entity = [[MWMNavigationDashboardEntity alloc] initWithFollowingInfo:info
                                                                  routePoints:@[]
                                                                         type:MWMRouterTypeVehicle];
    XCTAssertEqual(entity.timeToTarget, static_cast<NSUInteger>(seconds));
  }
}

- (void)testMissingManeuverKeepsRemainingRouteInfo
{
  routing::FollowingInfo info;
  info.m_distToTarget = platform::Distance(500.0);
  info.m_time = 120;
  info.m_completionPercent = 42.0;
  auto entity = [[MWMNavigationDashboardEntity alloc] initWithFollowingInfo:info
                                                                routePoints:@[]
                                                                       type:MWMRouterTypeBicycle];
  XCTAssertNil(entity.turnImage);
  XCTAssertNil(entity.nextTurnImage);
  XCTAssertNil(entity.distanceToTurn);
  XCTAssertNil(entity.turnUnits);
  XCTAssertTrue(entity.targetDistance.length > 0);
  XCTAssertEqual(entity.timeToTarget, 120u);
  XCTAssertEqual(entity.progress, 42.0);
}

- (void)testZeroDistanceManeuverStillHasADistance
{
  routing::FollowingInfo info;
  info.m_distToTurn = platform::Distance(0.0);
  auto entity = [[MWMNavigationDashboardEntity alloc] initWithFollowingInfo:info
                                                                routePoints:@[]
                                                                       type:MWMRouterTypePedestrian];
  XCTAssertTrue(entity.distanceToTurn.length > 0);
  XCTAssertTrue(entity.turnUnits.length > 0);
}

- (void)testTransitEstimateKeepsRawSeconds
{
  for (int seconds : kRawDurations)
  {
    TransitRouteInfo info;
    info.m_totalTimeInSec = seconds;
    auto entity = [[MWMNavigationDashboardEntity alloc] initWithTransitInfo:info];
    XCTAssertEqual(entity.timeToTarget, static_cast<NSUInteger>(seconds));
  }
}

@end
