#import "MWMZoomButtonsView.h"
#import "Common.h"
#import "MWMMapViewControlsCommon.h"

static CGFloat const kZoomViewOffsetToTopBound = 12.0;
static CGFloat const kZoomViewOffsetToBottomBound = 40.0;
static CGFloat const kZoomViewOffsetToFrameBound = 294.0;
static CGFloat const kZoomViewHideBoundPercent = 0.4;

@interface MWMZoomButtonsView ()

@property(nonatomic) CGRect defaultBounds;

@end

@implementation MWMZoomButtonsView

- (void)awakeFromNib
{
  self.defaultBounds = self.bounds;
  self.autoresizingMask = UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleHeight;
}

- (void)layoutSubviews
{
  self.bounds = self.defaultBounds;
  [self layoutXPosition:self.hidden];
  [self layoutYPosition];
  [super layoutSubviews];
}

- (void)layoutXPosition:(BOOL)hidden
{
  if (hidden)
    self.minX = self.superview.width;
  else
    self.maxX = self.superview.width - kViewControlsOffsetToBounds;
}

- (void)layoutYPosition
{
  CGFloat const maxY =
      MIN(self.superview.height - kZoomViewOffsetToFrameBound, self.bottomBound - kZoomViewOffsetToBottomBound);
  self.minY = MAX(maxY - self.height, self.topBound + kZoomViewOffsetToTopBound);
}

- (void)moveAnimated
{
  if (self.hidden)
    return;
  [UIView animateWithDuration:framesDuration(kMenuViewMoveFramesCount) animations:^{ [self layoutYPosition]; }];
}

- (void)fadeAnimatedIn:(BOOL)show
{
  [UIView animateWithDuration:framesDuration(kMenuViewHideFramesCount) animations:^{ self.alpha = show ? 1.0 : 0.0; }];
}

- (void)animate
{
  CGFloat const hideBound = kZoomViewHideBoundPercent * self.superview.height;
  BOOL const isHidden = self.alpha == 0.0;
  BOOL const willHide =
      (self.bottomBound < hideBound) || (self.defaultBounds.size.height > self.bottomBound - self.topBound);
  if (willHide)
  {
    if (!isHidden)
      [self fadeAnimatedIn:NO];
  }
  else
  {
    [self moveAnimated];
    if (isHidden)
      [self fadeAnimatedIn:YES];
  }
}

#pragma mark - Properties

- (void)setHidden:(BOOL)hidden animated:(BOOL)animated
{
  if (animated)
  {
    if (self.hidden == hidden)
      return;
    if (!hidden)
      self.hidden = NO;
    [self layoutXPosition:!hidden];
    [UIView animateWithDuration:framesDuration(kMenuViewHideFramesCount)
        animations:^{ [self layoutXPosition:hidden]; }
        completion:^(BOOL finished) {
          if (hidden)
            self.hidden = YES;
        }];
  }
  else
  {
    self.hidden = hidden;
  }
}

@synthesize topBound = _topBound;

- (void)setTopBound:(CGFloat)topBound
{
  if (equalScreenDimensions(_topBound, topBound))
    return;
  _topBound = topBound;
  [self animate];
}

- (CGFloat)topBound
{
  return MAX(0.0, _topBound);
}

@synthesize bottomBound = _bottomBound;

- (void)setBottomBound:(CGFloat)bottomBound
{
  if (equalScreenDimensions(_bottomBound, bottomBound))
    return;
  _bottomBound = bottomBound;
  [self animate];
}

- (CGFloat)bottomBound
{
  if (!self.superview)
    return _bottomBound;
  BOOL const isPortrait = self.superview.width < self.superview.height;
  CGFloat limit = IPAD ? 320.0 : isPortrait ? 200.0 : 80.0;
  return MIN(self.superview.height - limit, _bottomBound);
}

@end
