#import <CoreGraphics/CoreGraphics.h>
#import <Foundation/Foundation.h>
#import "MWMRouterType.h"

NS_ASSUME_NONNULL_BEGIN

@interface DeepLinkRouteStrategyAdapter : NSObject

@property(nonatomic, readonly) MWMRoutePoint * start;
@property(nonatomic, readonly) MWMRoutePoint * finish;
@property(nonatomic, readonly) NSArray<MWMRoutePoint *> * intermediatePoints;
@property(nonatomic, readonly) BOOL optimizeRoutePoints;
@property(nonatomic, readonly) BOOL startRouteNavigation;
@property(nonatomic, readonly) CGPoint startDirection;
@property(nonatomic, readonly) MWMRouterType type;

// Reads the route itinerary from the current parsed API state (populated by
// DeepLinkParser.parseAndSetApiURL); returns nil when there is no valid route.
- (nullable instancetype)init;

@end

NS_ASSUME_NONNULL_END
