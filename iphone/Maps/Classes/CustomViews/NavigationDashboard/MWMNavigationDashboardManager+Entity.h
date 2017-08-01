#import "MWMNavigationDashboardManager.h"

#include "platform/location.hpp"

@interface MWMNavigationDashboardManager (Entity)

- (void)updateFollowingInfo:(location::FollowingInfo const &)info;

@end
