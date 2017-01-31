#import "MWMCircularProgress.h"
#import "MWMNavigationDashboardInfoProtocol.h"
#import "MWMNavigationView.h"

@class MWMNavigationDashboardEntity;
@class MWMNavigationDashboardManager;
@class MWMTaxiCollectionView;

@interface MWMRoutePreview : MWMNavigationView<MWMNavigationDashboardInfoProtocol>

@property(weak, nonatomic, readonly) IBOutlet UIButton * extendButton;
@property(weak, nonatomic, readonly) IBOutlet MWMTaxiCollectionView * taxiCollectionView;
@property(weak, nonatomic) MWMNavigationDashboardManager * dashboardManager;

- (void)statePrepare;
- (void)stateError;
- (void)stateReady;
- (void)reloadData;
- (void)selectRouter:(MWMRouterType)routerType;
- (void)router:(MWMRouterType)routerType setState:(MWMCircularProgressState)state;
- (void)router:(MWMRouterType)routerType setProgress:(CGFloat)progress;

@end
