#import "MWMNavigationView.h"

#include "routing/router.hpp"

@protocol MWMRoutePreviewDataSource <NSObject>

@required
- (NSString *)source;
- (NSString *)destination;

@end

@class MWMNavigationDashboardEntity;
@class MWMRouteTypeButton;
@class MWMNavigationDashboardManager;
@class MWMCircularProgress;

@interface MWMRoutePreview : MWMNavigationView

@property (weak, nonatomic, readonly) IBOutlet UIButton * extendButton;
@property (weak, nonatomic) id<MWMRoutePreviewDataSource> dataSource;
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
