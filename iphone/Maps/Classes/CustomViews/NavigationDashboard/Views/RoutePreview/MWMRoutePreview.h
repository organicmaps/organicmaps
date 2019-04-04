#import "MWMCircularProgressState.h"
#import "MWMRouterType.h"

typedef NS_ENUM(NSInteger, MWMDrivingOptionsState) {
  MWMDrivingOptionsStateNone,
  MWMDrivingOptionsStateDefine,
  MWMDrivingOptionsStateChange
};

@class MWMNavigationDashboardEntity;
@class MWMNavigationDashboardManager;
@class MWMTaxiCollectionView;
@class MWMRoutePreview;

@protocol MWMRoutePreviewDelegate

- (void)routePreviewDidPressDrivingOptions:(MWMRoutePreview *)routePreview;

@end

@interface MWMRoutePreview : UIView

@property(nonatomic) MWMDrivingOptionsState drivingOptionsState;
@property(weak, nonatomic) id<MWMRoutePreviewDelegate> delegate;

- (void)addToView:(UIView *)superview;
- (void)remove;

- (void)statePrepare;
- (void)selectRouter:(MWMRouterType)routerType;
- (void)router:(MWMRouterType)routerType setState:(MWMCircularProgressState)state;
- (void)router:(MWMRouterType)routerType setProgress:(CGFloat)progress;

@end
