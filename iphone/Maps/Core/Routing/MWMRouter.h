#import "MWMRoutePoint.h"
#import "MWMRouterType.h"

#import <CoreApi/MWMTypes.h>

@class RouteElevationPreviewData;

typedef NS_ENUM(NSInteger, MWMRoadType) {
  MWMRoadTypeToll,
  MWMRoadTypeDirty,
  MWMRoadTypeFerry,
  MWMRoadTypeMotorway
};

/// Why a follow-track request was refused; mirrors the core FollowTrackResult. Everything but
/// Ok leaves the routing session exactly as it was.
typedef NS_ENUM(NSInteger, MWMFollowTrackResult) {
  MWMFollowTrackResultOk,
  /// The track does not exist (anymore).
  MWMFollowTrackResultNoTrack,
  /// The track has no two distinct points to follow.
  MWMFollowTrackResultDegenerateTrack,
  /// A profile a track can never be followed with (transit, ruler).
  MWMFollowTrackResultUnsupportedProfile,
  /// Called before the core was initialized.
  MWMFollowTrackResultNotInitialized,
  MWMFollowTrackResultBusy,
  MWMFollowTrackResultInvalidSelection,
  MWMFollowTrackResultNoRemainingGeometry,
};

NS_ASSUME_NONNULL_BEGIN

/// Everything the UI needs about the currently followed track, captured when the follow started.
/// A snapshot on purpose: the session outlives its track — navigation continues on the frozen
/// geometry after the track is deleted — so naming the track needs an existence check of its own.
@interface MWMFollowedTrackInfo : NSObject

@property(nonatomic, readonly) MWMTrackID trackId;
@property(copy, nonatomic, readonly) NSString * name;
@property(nonatomic, readonly) double lengthMeters;
@property(nonatomic, readonly) BOOL isReversed;
/// The point the follow ends at: the track's back, or its front when reversed.
@property(nonatomic, readonly) MWMRoutePoint * finishPoint;

@end

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
/// The router type to present and to gate on: the follow profile (never ruler/publicTransport)
/// while a track is being followed, the current router type otherwise.
+ (MWMRouterType)renderType;
+ (NSString *)stringFromRouterType:(MWMRouterType)type;

/// Captures the selected point and previews the remaining connected track section.
/// Refusals preserve the current session; accepted builds report through normal routing events.
+ (MWMFollowTrackResult)navigateTrack:(MWMTrackID)trackId;
/// Reverses a built preview at the captured source entry; active navigation is refused.
+ (MWMFollowTrackResult)reverseFollowedTrack;
/// Switches the preview profile without changing the persisted default. Refusals preserve the session.
+ (MWMFollowTrackResult)setFollowTrackProfile:(MWMRouterType)type;
+ (BOOL)isFollowingTrack;
/// nil when no track is being followed.
+ (nullable MWMFollowedTrackInfo *)followedTrackInfo;

+ (void)disableFollowMode;

+ (void)enableTurnNotifications:(BOOL)active;
+ (BOOL)areTurnNotificationsEnabled;
+ (void)setTurnNotificationsLocale:(NSString *)locale;
+ (NSArray<NSString *> *)turnNotifications;

+ (void)addPoint:(MWMRoutePoint *)point;
+ (void)addPointAndRebuild:(MWMRoutePoint *)point;
+ (void)continueRouteToPointAndRebuild:(MWMRoutePoint *)point;
+ (void)removePointAndRebuild:(MWMRoutePoint *)point;
+ (void)replacePointAndRebuild:(MWMRoutePoint *)point withPoint:(MWMRoutePoint *)newPoint;
+ (void)reverseRoute;
+ (void)removePoints;

+ (void)buildFromPoint:(MWMRoutePoint *)start bestRouter:(BOOL)bestRouter;
+ (void)buildToPoint:(MWMRoutePoint *)finish bestRouter:(BOOL)bestRouter;
+ (void)buildApiRouteWithType:(MWMRouterType)type
                   startPoint:(MWMRoutePoint *)startPoint
                  finishPoint:(MWMRoutePoint *)finishPoint;
+ (void)rebuildWithBestRouter:(BOOL)bestRouter;

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
