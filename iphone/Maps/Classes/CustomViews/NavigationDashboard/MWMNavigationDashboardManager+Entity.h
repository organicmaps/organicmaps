#import "MWMNavigationDashboardManager.h"

namespace location
{
struct FollowingInfo;
}

@interface MWMNavigationDashboardManager (Entity)

- (void)updateFollowingInfo:(location::FollowingInfo const &)info;

@end
