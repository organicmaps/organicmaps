#import <Foundation/Foundation.h>
#import "MWMRouterType.h"

NS_ASSUME_NONNULL_BEGIN

@class MWMRoutePoint;
@interface DeepLinkRouteStrategyAdapter : NSObject

// The itinerary itself is applied by the core (Framework::ExecuteRouteApiRequest);
// start/finish are exposed for validation and tests only.
@property(nonatomic, readonly) MWMRoutePoint * start;
@property(nonatomic, readonly) MWMRoutePoint * finish;
@property(nonatomic, readonly) BOOL startRouteNavigation;
@property(nonatomic, readonly) MWMRouterType type;

// Reads the route itinerary from the current parsed API state (populated by
// DeepLinkParser.parseAndSetApiURL); returns nil when there is no valid route.
- (nullable instancetype)init;

@end

NS_ASSUME_NONNULL_END
