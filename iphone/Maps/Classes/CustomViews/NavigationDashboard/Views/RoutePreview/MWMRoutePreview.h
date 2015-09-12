#import "MWMNavigationView.h"

@class MWMNavigationDashboardEntity;

@interface MWMRoutePreview : MWMNavigationView

@property (weak, nonatomic) IBOutlet UILabel * status;
@property (weak, nonatomic) IBOutlet UIButton * pedestrian;
@property (weak, nonatomic) IBOutlet UIButton * vehicle;
@property (weak, nonatomic) IBOutlet UILabel * timeLabel;
@property (weak, nonatomic) IBOutlet UILabel * distanceLabel;
@property (weak, nonatomic) IBOutlet UILabel * arrivalsLabel;
@property (weak, nonatomic) IBOutlet UIButton * cancelButton;

- (void)configureWithEntity:(MWMNavigationDashboardEntity *)entity;
- (void)statePlaning;
- (void)stateError;

- (void)setRouteBuildingProgress:(CGFloat)progress;
- (void)showGoButtonAnimated:(BOOL)show;

@end
