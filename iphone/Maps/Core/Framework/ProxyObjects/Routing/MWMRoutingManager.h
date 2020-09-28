#import "MWMRouterType.h"
#import "MWMRoutePoint.h"
#import "MWMRouterResultCode.h"
#import "MWMSpeedCameraManagerMode.h"
@class RouteInfo;

NS_ASSUME_NONNULL_BEGIN

NS_SWIFT_NAME(RoutingManagerListener)
@protocol MWMRoutingManagerListener <NSObject>
- (void)processRouteBuilderEventWithCode:(MWMRouterResultCode)code
                               countries:(NSArray<NSString *> *)absentCountries;
- (void)didLocationUpdate:(NSArray<NSString *> *)notifications;
- (void)updateCameraInfo:(BOOL)isCameraOnRoute speedLimit:(nullable NSString *)limit NS_SWIFT_NAME(updateCameraInfo(isCameraOnRoute:speedLimit:));
@end

NS_SWIFT_NAME(RoutingManager)
@interface MWMRoutingManager : NSObject
@property(class, nonatomic, readonly) MWMRoutingManager *routingManager;
@property(nonatomic, readonly, nullable) MWMRoutePoint *startPoint;
@property(nonatomic, readonly, nullable) MWMRoutePoint *endPoint;
@property(nonatomic, readonly) BOOL isOnRoute;
@property(nonatomic, readonly) BOOL isRoutingActive;
@property(nonatomic, readonly) BOOL isRouteFinished;
@property(nonatomic, readonly, nullable) RouteInfo *routeInfo;
@property(nonatomic, readonly) MWMRouterType type;
@property(nonatomic) MWMSpeedCameraManagerMode speedCameraMode;

- (instancetype)init NS_UNAVAILABLE;
- (void)addListener:(id<MWMRoutingManagerListener>)listener;
- (void)removeListener:(id<MWMRoutingManagerListener>)listener;

- (void)stopRoutingAndRemoveRoutePoints:(BOOL)flag;
- (void)deleteSavedRoutePoints;
- (void)applyRouterType:(MWMRouterType)type NS_SWIFT_NAME(apply(routeType:));
- (void)addRoutePoint:(MWMRoutePoint *)point NS_SWIFT_NAME(add(routePoint:));
- (void)buildRouteWithDidFailError:(NSError **)errorPtr __attribute__((swift_error(nonnull_error))) NS_SWIFT_NAME(buildRoute());
- (void)startRoute;
- (void)setOnNewTurnCallback:(MWMVoidBlock)callback;
- (void)resetOnNewTurnCallback;

- (instancetype)init __attribute__((unavailable("call +routingManager instead")));
- (instancetype)copy __attribute__((unavailable("call +routingManager instead")));
- (instancetype)copyWithZone:(NSZone *)zone __attribute__((unavailable("call +routingManager instead")));
+ (instancetype)allocWithZone:(struct _NSZone *)zone
__attribute__((unavailable("call +routingManager instead")));
+ (instancetype) new __attribute__((unavailable("call +routingManager instead")));
@end

NS_ASSUME_NONNULL_END
