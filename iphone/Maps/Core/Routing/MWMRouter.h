@class MWMRoutePoint;

typedef void (^MWMImageHeightBlock)(UIImage *, NSString *);

@interface MWMRouter : NSObject

+ (MWMRouter *)router;

+ (BOOL)hasRouteAltitude;
+ (BOOL)isTaxi;
+ (void)startRouting;
+ (void)stopRouting;
+ (BOOL)isRoutingActive;

@property(nonatomic, readonly) MWMRoutePoint * startPoint;
@property(nonatomic, readonly) MWMRoutePoint * finishPoint;
@property(nonatomic) MWMRouterType type;

- (void)swapPointsAndRebuild;
- (void)buildFromPoint:(MWMRoutePoint *)start bestRouter:(BOOL)bestRouter;
- (void)buildToPoint:(MWMRoutePoint *)finish bestRouter:(BOOL)bestRouter;
- (void)buildFromPoint:(MWMRoutePoint *)start
               toPoint:(MWMRoutePoint *)finish
            bestRouter:(BOOL)bestRouter;
- (void)rebuildWithBestRouter:(BOOL)bestRouter;
- (void)routeAltitudeImageForSize:(CGSize)size completion:(MWMImageHeightBlock)block;

- (instancetype)init __attribute__((unavailable("call +router instead")));
- (instancetype)copy __attribute__((unavailable("call +router instead")));
- (instancetype)copyWithZone:(NSZone *)zone __attribute__((unavailable("call +router instead")));
+ (instancetype)alloc __attribute__((unavailable("call +router instead")));
+ (instancetype)allocWithZone:(struct _NSZone *)zone
    __attribute__((unavailable("call +router instead")));
+ (instancetype) new __attribute__((unavailable("call +router instead")));

@end
