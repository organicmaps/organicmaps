#import "MWMNavigationDashboardManager.h"
#import "MWMRoutePoint.h"
#import "MWMRouterType.h"

#include "routing/turns.hpp"

namespace navigation_dashboard
{
NSString * TurnInstruction(routing::turns::CarDirection carTurn, routing::turns::PedestrianDirection pedestrianTurn,
                           uint8_t exitNumber, NSString * languageTag);
}  // namespace navigation_dashboard

namespace routing
{
class FollowingInfo;
}

struct TransitRouteInfo;

@interface MWMNavigationDashboardManager (Entity)

- (void)updateFollowingInfo:(routing::FollowingInfo const &)info
                routePoints:(NSArray<MWMRoutePoint *> *)points
                       type:(MWMRouterType)type;
- (void)updateTransitInfo:(TransitRouteInfo const &)info;

@end
