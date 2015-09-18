#import "MWMZoomButtonsView.h"
#import "MWMMapViewControlsCommon.h"

static CGFloat const kZoomViewOffsetToTopBound = 12.0;
static CGFloat const kZoomViewOffsetToBottomBound = 40.0;
static CGFloat const kZoomViewOffsetToFrameBound = 294.0;
static CGFloat const kZoomViewHideBoundPercent = 0.4;

@interface MWMZoomButtonsView()

@property (nonatomic) CGRect defaultBounds;

@end

@implementation MWMZoomButtonsView

- (instancetype)initWithCoder:(NSCoder *)aDecoder
{
  self = [super initWithCoder:aDecoder];
  if (self)
  {
    self.defaultBounds = self.bounds;
    self.autoresizingMask = UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleHeight;
    self.topBound = 0.0;
    self.bottomBound = 0.0;
  }
  return self;
}

- (void)layoutSubviews
{
  [super layoutSubviews];
  self.bounds = self.defaultBounds;
  [self layoutXPosition:self.hidden];
  [self layoutYPosition];
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
  CGFloat const maxY = MIN(self.superview.height - kZoomViewOffsetToFrameBound, self.bottomBound - kZoomViewOffsetToBottomBound);
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
  BOOL const willHide = (self.bottomBound < hideBound) || (self.defaultBounds.size.height > self.bottomBound - self.topBound);
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
  if (animated) {
    if (self.hidden == hidden)
      return;
    if (!hidden)
      self.hidden = NO;
    [self layoutXPosition:!hidden];
    [UIView animateWithDuration:framesDuration(kMenuViewHideFramesCount) animations:^
    {
      [self layoutXPosition:hidden];
    }
    completion:^(BOOL finished)
    {
      if (hidden)
        self.hidden = YES;
    }];
  }
  else
  {
    self.hidden = hidden;
  }
}

- (void)setTopBound:(CGFloat)topBound
{
  if (_topBound == topBound)
    return;
  _topBound = topBound;
  [self animate];
}

- (void)setBottomBound:(CGFloat)bottomBound
{
  if (_bottomBound == bottomBound)
    return;
  _bottomBound = bottomBound;
  [self animate];
}

@end
