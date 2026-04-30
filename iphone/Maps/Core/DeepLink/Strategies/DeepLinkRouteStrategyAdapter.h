#import <Foundation/Foundation.h>
#import "MWMRouterType.h"

NS_ASSUME_NONNULL_BEGIN

@interface DeepLinkRouteStrategyAdapter : NSObject

@property(nonatomic, readonly) MWMRoutePoint * start;
@property(nonatomic, readonly) MWMRoutePoint * finish;
@property(nonatomic, readonly) NSArray<MWMRoutePoint *> * intermediatePoints;
@property(nonatomic, readonly) BOOL optimizeRoutePoints;
@property(nonatomic, readonly) BOOL startRouteNavigation;
@property(nonatomic, readonly) MWMRouterType type;

// Requires a successfully parsed Route API request in the current core API state.
- (instancetype)init;

@end

NS_ASSUME_NONNULL_END
