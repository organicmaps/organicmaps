#import "MWMNavigationDashboardManager.h"

namespace routing
{
class FollowingInfo;
}

struct TransitRouteInfo;

@interface MWMNavigationDashboardManager (Entity)

- (void)updateFollowingInfo:(routing::FollowingInfo const &)info;
- (void)updateTransitInfo:(TransitRouteInfo const &)info;

@end
