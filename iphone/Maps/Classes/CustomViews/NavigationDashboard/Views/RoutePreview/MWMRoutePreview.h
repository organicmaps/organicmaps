#import "MWMNavigationView.h"

@class MWMNavigationDashboardEntity;
@class MWMRouteTypeButton;

@interface MWMRoutePreview : MWMNavigationView

@property (weak, nonatomic) IBOutlet MWMRouteTypeButton * pedestrian;
@property (weak, nonatomic) IBOutlet MWMRouteTypeButton * vehicle;

- (void)configureWithEntity:(MWMNavigationDashboardEntity *)entity;
- (void)statePlanning;
- (void)stateError;
- (void)stateReady;

- (void)setRouteBuildingProgress:(CGFloat)progress;

@end
