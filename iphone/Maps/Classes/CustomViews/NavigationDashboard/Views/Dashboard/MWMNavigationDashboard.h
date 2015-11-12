#import "MWMNavigationView.h"

@class MWMNavigationDashboardEntity;

@interface MWMNavigationDashboard : MWMNavigationView

@property (weak, nonatomic) IBOutlet UIImageView * direction;
@property (weak, nonatomic) IBOutlet UILabel * distanceToNextAction;
@property (weak, nonatomic) IBOutlet UILabel * distanceToNextActionUnits;
@property (weak, nonatomic) IBOutlet UILabel * distanceLeft;
@property (weak, nonatomic) IBOutlet UILabel * eta;
@property (weak, nonatomic) IBOutlet UILabel * arrivalsTimeLabel;
@property (weak, nonatomic) IBOutlet UILabel * roundRoadLabel;
@property (weak, nonatomic) IBOutlet UILabel * streetLabel;
@property (weak, nonatomic) IBOutlet UIButton * soundButton;
@property (weak, nonatomic) IBOutlet UISlider * progress;

- (void)configureWithEntity:(MWMNavigationDashboardEntity *)entity;

@end
