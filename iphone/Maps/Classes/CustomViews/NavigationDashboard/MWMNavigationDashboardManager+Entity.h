#import "MWMNavigationDashboardManager.h"

namespace location
{
class FollowingInfo;
}

@interface MWMNavigationDashboardManager (Entity)

- (void)updateFollowingInfo:(location::FollowingInfo const &)info;

@end
