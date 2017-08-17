#import "MWMRoutePoint.h"
#import "MWMRouterType.h"

typedef void (^MWMImageHeightBlock)(UIImage *, NSString *);

@interface MWMRouter : NSObject

+ (BOOL)isTaxi;
+ (BOOL)isRoutingActive;
+ (BOOL)isRouteBuilt;
+ (BOOL)isRouteFinished;
+ (BOOL)isRouteRebuildingOnly;
+ (BOOL)isOnRoute;

+ (BOOL)canAddIntermediatePoint;

+ (void)startRouting;
+ (void)stopRouting;

+ (NSArray<MWMRoutePoint *> *)points;
+ (NSInteger)pointsCount;
+ (MWMRoutePoint *)startPoint;
+ (MWMRoutePoint *)finishPoint;

+ (void)enableAutoAddLastLocation:(BOOL)enable;

+ (void)setType:(MWMRouterType)type;
+ (MWMRouterType)type;

+ (void)disableFollowMode;

+ (void)enableTurnNotifications:(BOOL)active;
+ (BOOL)areTurnNotificationsEnabled;
+ (void)setTurnNotificationsLocale:(NSString *)locale;
+ (NSArray<NSString *> *)turnNotifications;

+ (void)addPoint:(MWMRoutePoint *)point;
+ (void)removePoint:(MWMRoutePoint *)point;
+ (void)addPointAndRebuild:(MWMRoutePoint *)point;
+ (void)removePointAndRebuild:(MWMRoutePoint *)point;
+ (void)removePoints;

+ (void)buildFromPoint:(MWMRoutePoint *)start bestRouter:(BOOL)bestRouter;
+ (void)buildToPoint:(MWMRoutePoint *)finish bestRouter:(BOOL)bestRouter;
+ (void)buildApiRouteWithType:(MWMRouterType)type
                   startPoint:(MWMRoutePoint *)startPoint
                  finishPoint:(MWMRoutePoint *)finishPoint;
+ (void)rebuildWithBestRouter:(BOOL)bestRouter;

+ (BOOL)hasRouteAltitude;
+ (void)routeAltitudeImageForSize:(CGSize)size completion:(MWMImageHeightBlock)block;

+ (void)saveRouteIfNeeded;
+ (void)restoreRouteIfNeeded;

@end

@interface MWMRouter (RouteManager)

+ (void)openRouteManagerTransaction;
+ (void)applyRouteManagerTransaction;
+ (void)cancelRouteManagerTransaction;
+ (void)movePointAtIndex:(NSInteger)index toIndex:(NSInteger)newIndex;
+ (void)updatePreviewMode;

@end
