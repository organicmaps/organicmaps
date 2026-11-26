#import "MWMRoutePoint.h"
#import "MWMRouterType.h"

#import <CoreApi/MWMTypes.h>

@class RouteElevationPreviewData;

typedef NS_ENUM(NSInteger, MWMRoadType) {
  MWMRoadTypeToll,
  MWMRoadTypeDirty,
  MWMRoadTypeFerry,
  MWMRoadTypeMotorway,
  MWMRoadTypeSteps,
  MWMRoadTypeDifficultTrail
};

/// Ok means the request was accepted. Other values explain why the request was refused.
typedef NS_ENUM(NSInteger, MWMFollowTrackResult) {
  MWMFollowTrackResultOk,
  /// The track does not exist (anymore).
  MWMFollowTrackResultNoTrack,
  MWMFollowTrackResultBusy,
  MWMFollowTrackResultInvalidSelection,
  MWMFollowTrackResultNoRemainingGeometry,
};

NS_ASSUME_NONNULL_BEGIN

/// Captured track details remain valid after source deletion. Looking up the original track by
/// its ID requires an existence check.
@interface MWMFollowedTrackInfo : NSObject

@property(nonatomic, readonly) MWMTrackID trackId;
@property(copy, nonatomic, readonly) NSString * name;
/// Captured distance along the full source, independent of the map selection.
@property(nonatomic, readonly) double entryDistanceMeters;
/// Travel runs toward the start of the recorded section.
@property(nonatomic, readonly) BOOL isReversed;
/// The selected entry supports travel in the opposite direction.
@property(nonatomic, readonly) BOOL canReverse;
/// The destination borders a gap inside the recorded track.
@property(nonatomic, readonly) BOOL endsAtGap;

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
/// Selects an entry by distance along the full track. Near the track, inspection away from the
/// current join succeeds without starting a build. At the join a pick can select another visit.
/// Keeps the chosen direction unless only the opposite side of the entry has geometry.
/// Refusals leave the preview and selection unchanged.
+ (MWMFollowTrackResult)updateFollowedTrackEntryAtDistance:(double)distance
    NS_SWIFT_NAME(updateFollowedTrackEntry(atDistance:));
/// Reverses the completed track preview at the selected location. A closed trailhead switches chart endpoints.
+ (MWMFollowTrackResult)reverseFollowedTrack;
/// Switches the preview profile without changing the persisted default. Refusals preserve the session.
+ (MWMFollowTrackResult)setFollowTrackProfile:(MWMRouterType)type;
+ (BOOL)hasTrackNavigationSession;
/// nil when no track is being followed.
+ (nullable MWMFollowedTrackInfo *)followedTrackInfo;

+ (void)disableFollowMode;

+ (void)enableTurnNotifications:(BOOL)active;
+ (BOOL)areTurnNotificationsEnabled;
+ (void)setTurnNotificationsLocale:(NSString *)locale;
+ (NSArray<NSString *> *)turnNotifications;
/// Poll once per accepted location batch while following, before updating navigation controls.
+ (void)playGuidance;

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

+ (BOOL)saveRouteAsTrack;
+ (nullable RouteElevationPreviewData *)routeElevationProfileData;
+ (nullable NSNumber *)routeElevationActivePointDistance;
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
