#import "Common.h"
#import "MWMNavigationDashboard.h"
#import "MWMNavigationDashboardEntity.h"
#import "TimeUtils.h"
#import "UIColor+MapsMeColor.h"

@interface MWMImageView : UIImageView

@end

@implementation MWMImageView

- (void)setCenter:(CGPoint)center
{
//TODO(Vlad): There is hack for "cut" iOS7.
  if (isIOSVersionLessThan(8))
    return;
  [super setCenter:center];
}

@end

@implementation MWMNavigationDashboard

- (void)awakeFromNib
{
  [super awakeFromNib];
  if (IPAD)
    self.statusbarBackground = nil;
  [self.progress setThumbImage:[UIImage imageNamed:@"progress_circle_light"] forState:UIControlStateNormal];
  [self.progress setMaximumTrackTintColor:[UIColor blackOpaque]];
  [self.progress setMinimumTrackTintColor:[UIColor blackSecondaryText]];
  CALayer * l = self.layer;
  l.shouldRasterize = YES;
  l.rasterizationScale = UIScreen.mainScreen.scale;
}

- (void)configureWithEntity:(MWMNavigationDashboardEntity *)entity
{
  self.direction.image = entity.turnImage;
  if (!entity.isPedestrian)
    self.direction.transform = CGAffineTransformIdentity;
  self.distanceToNextAction.text = entity.distanceToTurn;
  self.distanceToNextActionUnits.text = entity.turnUnits;
  self.distanceLeft.text = [NSString stringWithFormat:@"%@ %@", entity.targetDistance, entity.targetUnits];
  self.eta.text = [NSDateFormatter estimatedArrivalTimeWithSeconds:@(entity.timeToTarget)];
  self.arrivalsTimeLabel.text = [NSDateFormatter localizedStringFromDate:[[NSDate date]
                                                 dateByAddingTimeInterval:entity.timeToTarget]
                                                 dateStyle:NSDateFormatterNoStyle timeStyle:NSDateFormatterShortStyle];
  self.roundRoadLabel.text = entity.roundExitNumber ? @(entity.roundExitNumber).stringValue : @"";
  self.streetLabel.text = entity.streetName;
  [self.progress setValue:entity.progress animated:YES];
}
#pragma mark - Properties

- (CGRect)defaultFrame
{
  if (IPAD)
    return {{self.leftBound + 12, self.topBound + 8}, {360., 186.}};
  return super.defaultFrame;
}

- (CGFloat)visibleHeight
{
  if (!IPAD)
    return super.visibleHeight;
  CGFloat height = 0;
  for (UIView * v in self.subviews)
  {
    if (!v.hidden)
      height += v.height;
  }
  return height;
}

@end
