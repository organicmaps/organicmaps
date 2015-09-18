#import "MWMMapViewControlsCommon.h"
#import "MWMSideMenuDownloadBadge.h"

@interface MWMSideMenuDownloadBadge ()

@property (weak, nonatomic) IBOutlet UIImageView * badgeBackground;
@property (weak, nonatomic) IBOutlet UILabel * downloadCount;

@end

@implementation MWMSideMenuDownloadBadge

- (void)layoutSubviews
{
  self.maxX = self.superview.width;
  self.badgeBackground.frame = self.bounds;
  self.downloadCount.frame = self.bounds;
  [super layoutSubviews];
}

- (void)showAnimatedAfterDelay:(NSTimeInterval)delay
{
  self.hidden = YES;
  dispatch_after(dispatch_time(DISPATCH_TIME_NOW, (int64_t)(delay * NSEC_PER_SEC)), dispatch_get_main_queue(), ^
  {
    [self expand];
  });
}

- (void)hide
{
  [self removeFromSuperview];
}

- (void)expand
{
  if (self.outOfDateCount == 0)
    return;
  self.hidden = NO;
  CATransform3D const zeroScale = CATransform3DScale(CATransform3DIdentity, 0.0, 0.0, 1.0);
  self.badgeBackground.layer.transform = self.downloadCount.layer.transform = zeroScale;
  self.badgeBackground.alpha = self.downloadCount.alpha = 0.0;
  self.badgeBackground.hidden = NO;
  self.downloadCount.hidden = NO;
  self.downloadCount.text = @(self.outOfDateCount).stringValue;
  [UIView animateWithDuration:framesDuration(4) animations:^
  {
    self.badgeBackground.layer.transform = self.downloadCount.layer.transform = CATransform3DIdentity;
    self.badgeBackground.alpha = self.downloadCount.alpha = 1.0;
  }];
}

@end
