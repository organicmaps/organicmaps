#import "Common.h"
#import "MWMSideButtonsView.h"
#import "MWMMapViewControlsCommon.h"

namespace
{
  CGFloat const kZoomViewOffsetToTopBound = 12.0;
  CGFloat const kZoomViewOffsetToBottomBound = 40.0;
  CGFloat const kZoomViewOffsetToFrameBound = 294.0;
  CGFloat const kZoomViewHideBoundPercent = 0.4;
} // namespace

@interface MWMSideButtonsView()

@property (nonatomic) CGRect defaultBounds;
@property (nonatomic) BOOL show;

@end

@implementation MWMSideButtonsView

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
  CGFloat const maxY = MIN(self.superview.height - kZoomViewOffsetToFrameBound, self.bottomBound - kZoomViewOffsetToBottomBound);
  self.minY = MAX(maxY - self.height, self.topBound + kZoomViewOffsetToTopBound);
}

- (void)moveAnimated
{
  if (self.hidden)
    return;
  [UIView animateWithDuration:framesDuration(kMenuViewMoveFramesCount) animations:^{ [self layoutYPosition]; }];
}

- (void)fadeAnimated
{
  [UIView animateWithDuration:framesDuration(kMenuViewHideFramesCount) animations:^{ self.alpha = self.show ? 1.0 : 0.0; }];
}

- (void)animate
{
  CGFloat const hideBound = kZoomViewHideBoundPercent * self.superview.height;
  BOOL const isHidden = self.alpha == 0.0;
  BOOL const willHide = (self.bottomBound < hideBound) || (self.defaultBounds.size.height > self.bottomBound - self.topBound);
  if (willHide)
  {
    if (!isHidden)
      self.show = NO;
  }
  else
  {
    [self moveAnimated];
    if (isHidden)
      self.show = YES;
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
  CGFloat limit = IPAD ? 320.0 : isPortrait ? 150.0 : 80.0;
  return MIN(self.superview.height - limit, _bottomBound);
}

- (void)setShow:(BOOL)show
{
  _show = show;
  [self fadeAnimated];
}

@end
