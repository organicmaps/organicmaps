#import "MWMNavigationView.h"

@protocol MWMRoutePreviewDataSource <NSObject>

@required
- (NSString *)source;
- (NSString *)destination;

@end

@class MWMNavigationDashboardEntity;
@class MWMRouteTypeButton;
@class MWMNavigationDashboardManager;

@interface MWMRoutePreview : MWMNavigationView

@property (weak, nonatomic) IBOutlet MWMRouteTypeButton * pedestrian;
@property (weak, nonatomic) IBOutlet MWMRouteTypeButton * vehicle;
@property (weak, nonatomic, readonly) IBOutlet UIButton * extendButton;
@property (weak, nonatomic) id<MWMRoutePreviewDataSource> dataSource;
@property (weak, nonatomic) MWMNavigationDashboardManager * dashboardManager;

- (void)configureWithEntity:(MWMNavigationDashboardEntity *)entity;
- (void)statePrepare;
- (void)statePlanning;
- (void)stateError;
- (void)stateReady;
- (void)setRouteBuildingProgress:(CGFloat)progress;
- (void)reloadData;

@end
