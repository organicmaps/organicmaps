#import "MWMNavigationView.h"

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
@property (nonatomic, readonly) MWMCircularProgress * pedestrianProgressView;
@property (nonatomic, readonly) MWMCircularProgress * vehicleProgressView;
@property (weak, nonatomic) id<MWMRoutePreviewDataSource> dataSource;
@property (weak, nonatomic) MWMNavigationDashboardManager * dashboardManager;

- (void)configureWithEntity:(MWMNavigationDashboardEntity *)entity;
- (void)statePrepare;
- (void)statePlanning;
- (void)stateError;
- (void)stateReady;
- (void)reloadData;
- (void)selectProgress:(MWMCircularProgress *)progress;

@end
