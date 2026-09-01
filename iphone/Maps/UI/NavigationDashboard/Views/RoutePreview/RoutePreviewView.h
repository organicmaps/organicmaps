#import "MWMCircularProgressState.h"
#import "MWMRoutePoint.h"
#import "MWMRouterType.h"

NS_ASSUME_NONNULL_BEGIN

typedef NS_ENUM(NSInteger, MWMDrivingOptionsState) {
  MWMDrivingOptionsStateNone,
  MWMDrivingOptionsStateDefine,
  MWMDrivingOptionsStateChange
};

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

    @protocol RouteNavigationControlsDelegate<NSObject>

- (void)ttsButtonDidTap;
- (void)settingsButtonDidTap;
- (void)stopRoutingButtonDidTap;
- (void)trackRecordingButtonDidTap;

@end

@protocol MWMRoutePreviewDelegate <RouteNavigationControlsDelegate>

- (void)routePreviewDidPressDrivingOptions;
- (void)routingStartButtonDidTap;
- (void)routePreviewDidSelect:(MWMRoutePointSelection *)selection;
- (void)routePreviewDidCancelPointSelection;

@end

NS_ASSUME_NONNULL_END
