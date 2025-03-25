#import "MWMCircularProgressState.h"
#import "MWMRouterType.h"

NS_ASSUME_NONNULL_BEGIN

typedef NS_ENUM(NSInteger, MWMDrivingOptionsState) {
  MWMDrivingOptionsStateNone,
  MWMDrivingOptionsStateDefine,
  MWMDrivingOptionsStateChange
};

@class MWMRoutePoint;

@protocol MWMRoutePreviewDelegate;
@protocol RoutePreviewView <NSObject>;

@property(nonatomic) MWMDrivingOptionsState drivingOptionsState;
@property(weak, nonatomic) id<MWMRoutePreviewDelegate> delegate;

- (void)addToView:(UIView *)superview;
- (void)remove;

- (void)statePrepare;
- (void)selectRouter:(MWMRouterType)routerType;
- (void)router:(MWMRouterType)routerType setState:(MWMCircularProgressState)state;
- (void)router:(MWMRouterType)routerType setProgress:(CGFloat)progress;

@end

@protocol RouteNavigationControlsDelegate <NSObject>

- (void)ttsButtonDidTap;
- (void)settingsButtonDidTap;
- (void)stopRoutingButtonDidTap;

@end

@protocol MWMRoutePreviewDelegate <RouteNavigationControlsDelegate>

- (void)routePreviewDidPressDrivingOptions;
- (void)routingStartButtonDidTap;
- (void)routePreviewDidSelectPoint:(MWMRoutePoint * _Nullable)point  shouldAppend:(BOOL)shouldAppend;

@end

NS_ASSUME_NONNULL_END
