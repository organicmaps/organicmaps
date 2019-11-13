#import <Foundation/Foundation.h>
#import "MWMRouterType.h"

NS_ASSUME_NONNULL_BEGIN

@class MWMRoutePoint;
@interface DeepLinkRouteStrategyAdapter : NSObject

@property(nonatomic, readonly) MWMRoutePoint* p1;
@property(nonatomic, readonly) MWMRoutePoint* p2;
@property(nonatomic, readonly) MWMRouterType type;

- (nullable instancetype)init:(NSURL*)url;

@end

NS_ASSUME_NONNULL_END
