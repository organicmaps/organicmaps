#include "MWMRoutePoint.h"

#include "routing/router.hpp"

@interface MWMRouter : NSObject

+ (MWMRouter *)router;

@property(nonatomic, readonly) MWMRoutePoint startPoint;
@property(nonatomic, readonly) MWMRoutePoint finishPoint;
@property(nonatomic) routing::RouterType type;

- (void)swapPointsAndRebuild;
- (void)buildFromPoint:(MWMRoutePoint const &)start bestRouter:(BOOL)bestRouter;
- (void)buildToPoint:(MWMRoutePoint const &)finish bestRouter:(BOOL)bestRouter;
- (void)buildFromPoint:(MWMRoutePoint const &)start
               toPoint:(MWMRoutePoint const &)finish
            bestRouter:(BOOL)bestRouter;
- (void)rebuildWithBestRouter:(BOOL)bestRouter;
- (void)start;
- (void)stop;
- (void)restore;

- (instancetype)init __attribute__((unavailable("call +router instead")));
- (instancetype)copy __attribute__((unavailable("call +router instead")));
- (instancetype)copyWithZone:(NSZone *)zone __attribute__((unavailable("call +router instead")));
+ (instancetype)alloc __attribute__((unavailable("call +router instead")));
+ (instancetype)allocWithZone:(struct _NSZone *)zone
    __attribute__((unavailable("call +router instead")));
+ (instancetype) new __attribute__((unavailable("call +router instead")));

@end
