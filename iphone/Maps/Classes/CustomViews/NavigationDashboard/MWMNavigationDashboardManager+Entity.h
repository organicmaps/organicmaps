#import "MWMNavigationDashboardManager.h"
#import "MWMRouterType.h"

namespace routing {
class FollowingInfo;
}

struct TransitRouteInfo;

@interface MWMNavigationDashboardManager (Entity)

- (void)updateFollowingInfo:(routing::FollowingInfo const &)info type:(MWMRouterType)type;
- (void)updateTransitInfo:(TransitRouteInfo const &)info;

@end
