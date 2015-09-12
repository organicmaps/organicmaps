#import "Common.h"
#import "MWMRouteHelperPanel.h"
#import "UIKitCategories.h"

static CGFloat const kHeight = 40.;

@implementation MWMRouteHelperPanel

- (void)setHidden:(BOOL)hidden
{
  if (IPAD)
  {
    if (!hidden)
      [super setHidden:hidden];
    [UIView animateWithDuration:kDefaultAnimationDuration animations:^
    {
      self.alpha = hidden ? 0. : 1.;
    }
    completion:^(BOOL finished)
    {
      if (hidden)
        [super setHidden:hidden];
    }];
    return;
  }
  if (hidden)
  {
    [UIView animateWithDuration:kDefaultAnimationDuration animations:^
    {
      self.alpha = 0.;
    }
    completion:^(BOOL finished)
    {
      [super setHidden:hidden];
    }];
  }
  else
  {
    [super setHidden:hidden];
    CGFloat const offsetFactor = 40.;
    CGFloat const scaleFactor = 0.4;
    self.transform = CGAffineTransformMakeScale(scaleFactor, scaleFactor);
    self.minY += offsetFactor;
    [UIView animateWithDuration:kDefaultAnimationDuration animations:^
    {
      self.transform = CGAffineTransformIdentity;
      self.alpha = 1.;
      self.minY -= offsetFactor;
    }];
  }
}

- (void)setTopBound:(CGFloat)topBound
{
  [self layoutIfNeeded];
  _topBound = topBound;
  self.minY = topBound;
  [UIView animateWithDuration:kDefaultAnimationDuration animations:^{ [self layoutIfNeeded]; }];
}

- (void)layoutSubviews
{
  self.height = self.defaultHeight;
  self.minY = self.topBound;
  if (IPAD)
    self.minX = 0.;
  else
    self.center = {self.parentView.center.x, self.center.y};
  [super layoutSubviews];
}

- (void)willMoveToSuperview:(UIView *)newSuperview
{
  [super willMoveToSuperview:newSuperview];
  self.alpha = 0.;
  self.hidden = YES;
}

- (CGFloat)defaultHeight
{
  return kHeight;
}

@end
