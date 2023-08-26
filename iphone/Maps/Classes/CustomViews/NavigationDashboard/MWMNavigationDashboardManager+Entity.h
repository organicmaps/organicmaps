#import "MWMNavigationDashboardManager.h"
#import "MWMRoutePoint.h"
#import "MWMRouterType.h"

namespace routing {
class FollowingInfo;
}

struct TransitRouteInfo;

@interface MWMNavigationDashboardManager (Entity)

- (void)updateFollowingInfo:(routing::FollowingInfo const &)info routePoints:(NSArray<MWMRoutePoint *> *)points type:(MWMRouterType)type;
- (void)updateTransitInfo:(TransitRouteInfo const &)info;

@end
