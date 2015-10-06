#import "MWMNavigationView.h"

@class MWMNavigationDashboardEntity;

@interface MWMRoutePreview : MWMNavigationView

@property (weak, nonatomic) IBOutlet UIButton * pedestrian;
@property (weak, nonatomic) IBOutlet UIButton * vehicle;

- (void)configureWithEntity:(MWMNavigationDashboardEntity *)entity;
- (void)statePlanning;
- (void)stateError;
- (void)stateReady;

- (void)setRouteBuildingProgress:(CGFloat)progress;

@end
