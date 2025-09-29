#import "MWMNavigationDashboardManager+Entity.h"
#import "MWMRouter.h"
#import "RouteInfo+Core.h"

#import <AudioToolbox/AudioServices.h>

@interface MWMNavigationDashboardManager ()

@property(copy, nonatomic) NSDictionary * etaAttributes;
@property(copy, nonatomic) NSDictionary * etaSecondaryAttributes;

- (void)onNavigationInfoUpdated:(RouteInfo *)entity;

@end

@implementation MWMNavigationDashboardManager (Entity)

- (void)updateFollowingInfo:(routing::FollowingInfo const &)info
                routePoints:(NSArray<MWMRoutePoint *> *)points
                       type:(MWMRouterType)type
{
  if ([MWMRouter isRouteFinished])
  {
    [MWMRouter stopRouting];
    AudioServicesPlaySystemSound(kSystemSoundID_Vibrate);
    return;
  }
  auto const routeInfo = [[RouteInfo alloc] initWithFollowingInfo:info routePoints:points type:type isCarPlay:NO];
  [self onNavigationInfoUpdated:routeInfo];
}

- (void)updateTransitInfo:(TransitRouteInfo const &)info
{
  auto const routeInfo = [[RouteInfo alloc] initWithTransitInfo:info];
  [self onNavigationInfoUpdated:routeInfo];
}

@end
