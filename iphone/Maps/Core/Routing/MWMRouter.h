#import "MWMRoutePoint.h"

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

+ (void)setType:(MWMRouterType)type;
+ (MWMRouterType)type;

+ (void)disableFollowMode;

+ (void)enableTurnNotifications:(BOOL)active;
+ (BOOL)areTurnNotificationsEnabled;
+ (void)setTurnNotificationsLocale:(NSString *)locale;
+ (NSArray<NSString *> *)turnNotifications;

+ (void)removeStartPointAndRebuild:(int8_t)intermediateIndex;
+ (void)removeFinishPointAndRebuild:(int8_t)intermediateIndex;
+ (void)addIntermediatePointAndRebuild:(MWMRoutePoint *)point
                     intermediateIndex:(int8_t)intermediateIndex;
+ (void)removeIntermediatePointAndRebuild:(int8_t)intermediateIndex;
+ (void)buildFromPoint:(MWMRoutePoint *)start bestRouter:(BOOL)bestRouter;
+ (void)buildToPoint:(MWMRoutePoint *)finish bestRouter:(BOOL)bestRouter;
+ (void)buildFromPoint:(MWMRoutePoint *)startPoint
               toPoint:(MWMRoutePoint *)finishPoint
            bestRouter:(BOOL)bestRouter;
+ (void)rebuildWithBestRouter:(BOOL)bestRouter;

+ (BOOL)hasRouteAltitude;
+ (void)routeAltitudeImageForSize:(CGSize)size completion:(MWMImageHeightBlock)block;

@end
