#import "RouteInfo.h"
#import "MWMRouterType.h"

#include "routing/following_info.hpp"
#include "map/transit/transit_display.hpp"

@class MWMRoutePoint;

@interface RouteInfo (Core)

- (instancetype)initWithFollowingInfo:(routing::FollowingInfo const &)info
                          routePoints:(NSArray<MWMRoutePoint *> *)points
                                 type:(MWMRouterType)type
                            isCarPlay:(BOOL)isCarPlay;
- (instancetype)initWithTransitInfo:(TransitRouteInfo const &)info;

@end
