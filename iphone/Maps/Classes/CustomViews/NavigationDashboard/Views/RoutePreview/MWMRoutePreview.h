#import "MWMCircularProgressState.h"
#import "MWMRouterType.h"

@class MWMNavigationDashboardEntity;
@class MWMNavigationDashboardManager;
@class MWMTaxiCollectionView;

@interface MWMRoutePreview : UIView

- (void)addToView:(UIView *)superview;
- (void)remove;

- (void)statePrepare;
- (void)selectRouter:(MWMRouterType)routerType;
- (void)router:(MWMRouterType)routerType setState:(MWMCircularProgressState)state;
- (void)router:(MWMRouterType)routerType setProgress:(CGFloat)progress;

@end
