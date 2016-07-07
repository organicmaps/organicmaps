#import "MWMNavigationView.h"

#include "routing/router.hpp"

@class MWMNavigationDashboardEntity;
@class MWMNavigationDashboardManager;
@class MWMCircularProgress;

@interface MWMRoutePreview : MWMNavigationView

@property (weak, nonatomic, readonly) IBOutlet UIButton * extendButton;
@property (weak, nonatomic) MWMNavigationDashboardManager * dashboardManager;

- (void)configureWithEntity:(MWMNavigationDashboardEntity *)entity;
- (void)statePrepare;
- (void)statePlanning;
- (void)stateError;
- (void)stateReady;
- (void)reloadData;
- (void)selectRouter:(routing::RouterType)routerType;
- (void)router:(routing::RouterType)routerType setState:(MWMCircularProgressState)state;
- (void)router:(routing::RouterType)routerType setProgress:(CGFloat)progress;

@end
