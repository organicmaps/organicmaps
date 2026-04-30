#import <CoreGraphics/CoreGraphics.h>
#import "MWMRoutePoint.h"
#import "MWMRouterType.h"

@class RouteElevationPreviewData;

typedef NS_ENUM(NSInteger, MWMRoadType) {
  MWMRoadTypeToll,
  MWMRoadTypeDirty,
  MWMRoadTypeFerry,
  MWMRoadTypeMotorway
};

NS_ASSUME_NONNULL_BEGIN

@interface MWMRouter : NSObject

+ (void)subscribeToEvents;
+ (void)unsubscribeFromEvents;

+ (BOOL)isRoutingActive;
+ (BOOL)isRouteBuilt;
+ (BOOL)isRouteValid;
+ (BOOL)isRouteFinished;
+ (BOOL)isRouteRebuildingOnly;
+ (BOOL)isOnRoute;

+ (BOOL)isSpeedCamLimitExceeded;

+ (BOOL)canAddIntermediatePoint;
+ (BOOL)isRoutePointsLimitReached;

+ (void)startRouting;
+ (void)stopRouting;

+ (NSArray<MWMRoutePoint *> *)points;
+ (NSInteger)pointsCount;
+ (nullable MWMRoutePoint *)startPoint;
+ (nullable MWMRoutePoint *)finishPoint;

+ (void)enableAutoAddLastLocation:(BOOL)enable;

+ (void)setType:(MWMRouterType)type;
+ (MWMRouterType)type;
+ (NSString *)stringFromRouterType:(MWMRouterType)type;

+ (void)disableFollowMode;

+ (void)enableTurnNotifications:(BOOL)active;
+ (BOOL)areTurnNotificationsEnabled;
+ (void)setTurnNotificationsLocale:(NSString *)locale;
+ (NSArray<NSString *> *)turnNotifications;

+ (void)addPoint:(MWMRoutePoint *)point;
+ (void)removePoint:(MWMRoutePoint *)point;
+ (void)addPointAndRebuild:(MWMRoutePoint *)point;
+ (void)continueRouteToPointAndRebuild:(MWMRoutePoint *)point;
+ (void)removePointAndRebuild:(MWMRoutePoint *)point;
+ (void)replacePointAndRebuild:(MWMRoutePoint *)point withPoint:(MWMRoutePoint *)newPoint;
+ (void)reverseRoute;
+ (void)removePoints;

+ (void)buildFromPoint:(MWMRoutePoint *)start bestRouter:(BOOL)bestRouter;
+ (void)buildToPoint:(MWMRoutePoint *)finish bestRouter:(BOOL)bestRouter;
+ (void)buildApiRouteWithType:(MWMRouterType)type startRouteNavigation:(BOOL)startRouteNavigation;
+ (void)buildApiRouteWithType:(MWMRouterType)type
                   startPoint:(MWMRoutePoint *)startPoint
                  finishPoint:(MWMRoutePoint *)finishPoint;
+ (void)buildApiRouteWithType:(MWMRouterType)type
                   startPoint:(MWMRoutePoint *)startPoint
           intermediatePoints:(NSArray<MWMRoutePoint *> *)intermediatePoints
                  finishPoint:(MWMRoutePoint *)finishPoint
          optimizeRoutePoints:(BOOL)optimizeRoutePoints;
+ (void)buildApiRouteWithType:(MWMRouterType)type
                   startPoint:(MWMRoutePoint *)startPoint
           intermediatePoints:(NSArray<MWMRoutePoint *> *)intermediatePoints
                  finishPoint:(MWMRoutePoint *)finishPoint
          optimizeRoutePoints:(BOOL)optimizeRoutePoints
         startRouteNavigation:(BOOL)startRouteNavigation;
+ (void)buildApiRouteWithType:(MWMRouterType)type
                   startPoint:(MWMRoutePoint *)startPoint
           intermediatePoints:(NSArray<MWMRoutePoint *> *)intermediatePoints
                  finishPoint:(MWMRoutePoint *)finishPoint
          optimizeRoutePoints:(BOOL)optimizeRoutePoints
         startRouteNavigation:(BOOL)startRouteNavigation
               startDirection:(CGPoint)startDirection;
+ (void)rebuildWithBestRouter:(BOOL)bestRouter;
+ (void)rebuildWithBestRouter:(BOOL)bestRouter startDirection:(CGPoint)startDirection;

/// Builds an openable URL from a deep-link callback/back string that the core already
/// URL-decoded, re-encoding dangling '%' and other invalid characters before parsing.
+ (nullable NSURL *)callbackURLFromString:(NSString *)callbackString;

+ (BOOL)hasRouteAltitude;
+ (void)saveRouteAsTrack;
+ (nullable RouteElevationPreviewData *)routeElevationProfileData;
+ (void)setRouteElevationActivePointDistance:(double)distance;
+ (void)resetRouteElevationActivePoint;

+ (void)saveRouteIfNeeded;
+ (void)restoreRouteIfNeeded;
+ (BOOL)hasSavedRoute;
+ (BOOL)isRestoreProcessCompleted;

+ (void)updateRoute;
+ (BOOL)hasActiveDrivingOptions;
+ (void)avoidRoadTypeAndRebuild:(MWMRoadType)type;
+ (void)optimizeRoutePointsAndRebuild;
+ (void)showNavigationMapControls;
+ (void)hideNavigationMapControls;

- (instancetype)init __attribute__((unavailable("call +router instead")));
- (instancetype)copy __attribute__((unavailable("call +router instead")));
- (instancetype)copyWithZone:(NSZone *)zone __attribute__((unavailable("call +router instead")));
+ (instancetype)allocWithZone:(struct _NSZone *)zone __attribute__((unavailable("call +router instead")));
+ (instancetype)new __attribute__((unavailable("call +router instead")));

@end

@interface MWMRouter (RouteManager)

+ (void)openRouteManagerTransaction;
+ (void)applyRouteManagerTransaction;
+ (void)cancelRouteManagerTransaction;
+ (void)movePointAtIndex:(NSInteger)index toIndex:(NSInteger)newIndex;
+ (void)updatePreviewMode;

@end

NS_ASSUME_NONNULL_END
