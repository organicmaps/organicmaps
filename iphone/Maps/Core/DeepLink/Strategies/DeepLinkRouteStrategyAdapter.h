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

- (nullable instancetype)init:(NSURL *)url;

@end

NS_ASSUME_NONNULL_END
