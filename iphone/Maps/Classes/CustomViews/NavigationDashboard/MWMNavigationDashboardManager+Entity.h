#import "MWMNavigationDashboardManager.h"

namespace location
{
class FollowingInfo;
}

struct TransitRouteInfo;

@interface MWMNavigationDashboardManager (Entity)

- (void)updateFollowingInfo:(location::FollowingInfo const &)info;
- (void)updateTransitInfo:(TransitRouteInfo const &)info;

@end
