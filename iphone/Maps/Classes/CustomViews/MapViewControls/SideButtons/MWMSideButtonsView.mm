#import "Common.h"
#import "MWMButton.h"
#import "MWMMapViewControlsCommon.h"
#import "MWMSideButtonsView.h"

namespace
{
  CGFloat bottomBoundLimit(BOOL isPortrait)
  {
    if (IPAD)
      return isPortrait ? 320.0 : 240.0;
    else
      return isPortrait ? 180.0 : 60.0;
  }

  CGFloat zoom2LayoutOffset(BOOL isPortrait)
  {
    return IPAD || isPortrait ? 52.0 : 30;
  }
} // namespace

@interface MWMSideButtonsView()

@property (nonatomic) CGRect defaultBounds;

@property (weak, nonatomic) IBOutlet MWMButton * zoomIn;
@property (weak, nonatomic) IBOutlet MWMButton * zoomOut;
@property (weak, nonatomic) IBOutlet MWMButton * location;

@property (nonatomic) BOOL isPortrait;

@end

@implementation MWMSideButtonsView

- (void)awakeFromNib
{
  self.defaultBounds = self.bounds;
  self.autoresizingMask = UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleHeight;
}

- (void)layoutSubviews
{
  BOOL const isPortrait = self.superview.width < self.superview.height;
  if (self.isPortrait != isPortrait && self.superview)
  {
    self.isPortrait = isPortrait;
    self.bottomBound = self.superview.height;
    self.location.minY = self.zoomOut.maxY + zoom2LayoutOffset(isPortrait);
    CGSize size = self.defaultBounds.size;
    size.height = self.location.maxY;
    self.defaultBounds.size = size;
  }
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
  self.maxY = self.bottomBound;
}

- (void)fadeZoomButtonsShow:(BOOL)show
{
  CGFloat const alpha = show ? 1.0 : 0.0;
  [UIView animateWithDuration:framesDuration(kMenuViewHideFramesCount) animations:^
  {
    self.zoomIn.alpha = alpha;
    self.zoomOut.alpha = alpha;
  }];
}

- (void)animate
{
  [self layoutYPosition];
  BOOL const isZoomHidden = self.zoomIn.alpha == 0.0;
  BOOL const willZoomHide = (self.defaultBounds.size.height > self.bottomBound - self.topBound);
  if (willZoomHide)
  {
    if (!isZoomHidden)
      [self fadeZoomButtonsShow:NO];
  }
  else
  {
    if (isZoomHidden)
      [self fadeZoomButtonsShow:YES];
  }
}

#pragma mark - Properties

- (void)setZoomHidden:(BOOL)zoomHidden
{
  _zoomHidden = zoomHidden;
  CGFloat const minX = zoomHidden ? self.width + kViewControlsOffsetToBounds : 0.0;
  self.zoomIn.minX = minX;
  self.zoomOut.minX = minX;
}

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
  return MIN(self.superview.height - bottomBoundLimit(isPortrait), _bottomBound);
}

@end
