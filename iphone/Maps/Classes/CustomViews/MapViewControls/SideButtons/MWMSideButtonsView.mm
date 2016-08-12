#import "Common.h"
#import "MWMButton.h"
#import "MWMMapViewControlsCommon.h"
#import "MWMSideButtonsView.h"

namespace
{
CGFloat constexpr kZoomOutToLayoutPortraitOffset = 52;
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
    CGFloat const zoomOutToLayoutOffset =
        IPAD || isPortrait ? kZoomOutToLayoutPortraitOffset : self.zoomOut.minY - self.zoomIn.maxY;
    self.location.minY = self.zoomOut.maxY + zoomOutToLayoutOffset;
    CGSize size = self.defaultBounds.size;
    size.height = self.location.maxY;
    self.defaultBounds.size = size;
  }
  self.bounds = self.defaultBounds;
  self.bottomBound = self.superview.height;

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

- (void)fadeLocationButtonShow:(BOOL)show
{
  [UIView animateWithDuration:framesDuration(kMenuViewHideFramesCount) animations:^
  {
    self.location.alpha = show ? 1.0 : 0.0;
  }];
}

- (void)animate
{
  [self layoutYPosition];

  CGFloat const spaceLeft = self.bottomBound - self.topBound -
                            (equalScreenDimensions(self.topBound, 0.0) ? statusBarHeight() : 0.0);
  BOOL const isZoomHidden = self.zoomIn.alpha == 0.0;
  BOOL const willZoomHide = (self.defaultBounds.size.height > spaceLeft);
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
  BOOL const isLocationHidden = self.location.alpha == 0.0;
  BOOL const willLocationHide = (self.location.height > spaceLeft);
  if (willLocationHide)
  {
    if (!isLocationHidden)
      [self fadeLocationButtonShow:NO];
  }
  else
  {
    if (isLocationHidden)
      [self fadeLocationButtonShow:YES];
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
  CGFloat const bottomBoundLimit =
      (self.superview.height - self.zoomOut.maxY) / 2 - (self.location.maxY - self.zoomOut.maxY);
  return MIN(self.superview.height - bottomBoundLimit, _bottomBound);
}

@end
