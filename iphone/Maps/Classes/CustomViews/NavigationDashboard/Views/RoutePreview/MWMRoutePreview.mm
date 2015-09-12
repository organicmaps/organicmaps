#import "Common.h"
#import "MWMCircularProgress.h"
#import "MWMNavigationDashboardEntity.h"
#import "MWMRoutePreview.h"
#import "TimeUtils.h"
#import "UIColor+MapsMeColor.h"
#import "UIKitCategories.h"

@interface MWMRoutePreview () <MWMCircularProgressDelegate>

@property (nonatomic) CGFloat goButtonHiddenOffset;
@property (weak, nonatomic) IBOutlet NSLayoutConstraint * goButtonVerticalOffset;
@property (weak, nonatomic) IBOutlet UIView * statusBox;
@property (weak, nonatomic) IBOutlet UIView * completeBox;
@property (weak, nonatomic) IBOutlet NSLayoutConstraint * goButtonHeight;
@property (weak, nonatomic) IBOutlet UIView * progress;
@property (weak, nonatomic) IBOutlet UIView * progressIndicator;
@property (nonatomic) BOOL showGoButton;
@property (nonatomic) MWMCircularProgress * progressManager;

@end

@implementation MWMRoutePreview

- (void)awakeFromNib
{
  [super awakeFromNib];
  self.goButtonHiddenOffset = self.goButtonVerticalOffset.constant;
}

- (void)configureWithEntity:(MWMNavigationDashboardEntity *)entity
{
  self.timeLabel.text = [NSDateFormatter estimatedArrivalTimeWithSeconds:@(entity.timeToTarget)];
  self.distanceLabel.text = [NSString stringWithFormat:@"%@ %@", entity.targetDistance, entity.targetUnits];
  NSString * arriveStr = [NSDateFormatter localizedStringFromDate:[[NSDate date]
                                           dateByAddingTimeInterval:entity.timeToTarget]
                                                          dateStyle:NSDateFormatterNoStyle
                                                          timeStyle:NSDateFormatterShortStyle];
  self.arrivalsLabel.text = [NSString stringWithFormat:@"%@ %@", L(@"routing_arrive"), arriveStr];
}

- (void)remove
{
  [super remove];
  self.pedestrian.enabled = YES;
  self.vehicle.enabled = YES;
}

- (void)statePlaning
{
  self.showGoButton = NO;
  self.statusBox.hidden = NO;
  self.completeBox.hidden = YES;
  [self.progressManager reset];
  self.progress.hidden = NO;
  self.cancelButton.hidden = YES;
  self.status.text = L(@"routing_planning");
  self.status.textColor = UIColor.blackHintText;
}

- (void)stateError
{
  self.progress.hidden = YES;
  self.cancelButton.hidden = NO;
  self.status.text = L(@"routing_planning_error");
  self.status.textColor = UIColor.red;
}

- (void)showGoButtonAnimated:(BOOL)show
{
  [self layoutIfNeeded];
  self.showGoButton = show;
  [UIView animateWithDuration:kDefaultAnimationDuration animations:^{ [self layoutIfNeeded]; }];
}

#pragma mark - Properties

- (void)setShowGoButton:(BOOL)showGoButton
{
  _showGoButton = showGoButton;
  self.goButtonVerticalOffset.constant = showGoButton ? 0.0 : self.goButtonHiddenOffset;
  self.statusBox.hidden = YES;
  self.completeBox.hidden = NO;
  self.progress.hidden = YES;
  self.cancelButton.hidden = NO;
}

- (CGFloat)visibleHeight
{
  CGFloat height = super.visibleHeight;
  if (self.showGoButton)
    height += self.goButtonHeight.constant;
  return height;
}

- (void)setRouteBuildingProgress:(CGFloat)progress
{
  dispatch_async(dispatch_get_main_queue(), ^
  {
    self.progressManager.progress = progress / 100.;
  });
}

- (MWMCircularProgress *)progressManager
{
  if (!_progressManager)
    _progressManager = [[MWMCircularProgress alloc] initWithParentView:self.progressIndicator delegate:self];
  return _progressManager;
}

- (void)progressButtonPressed:(nonnull MWMCircularProgress *)progress
{
  [self.cancelButton sendActionsForControlEvents:UIControlEventTouchUpInside];
}

@end
